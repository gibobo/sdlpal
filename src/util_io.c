#include "util.h"
#include "profile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <windows.h>
#define UTIL_FILENO _fileno
#define UTIL_LSEEK _lseek
#define UTIL_READ(fd, buffer, size) _read((fd), (buffer), (unsigned)(size))
#define UTIL_WRITE(fd, buffer, size) _write((fd), (buffer), (unsigned)(size))
#else
#include <sys/stat.h>
#include <unistd.h>
#define UTIL_FILENO fileno
#define UTIL_LSEEK lseek
#define UTIL_READ read
#define UTIL_WRITE write
#endif
#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

// Reads and writes of at least this size go through whole sectors where the
// SD card was measured to gain from it (the P4); the desktop takes the same path
// so the tests check its bytes. The other boards keep plain stream transfers.
#if UTIL_PLATFORM_P4 || !defined(ARDUINO_ARCH_ESP32)
#define UTIL_SECTOR_IO 1
#else
#define UTIL_SECTOR_IO 0
#endif
#define UTIL_SECTOR_IO_MIN 4096u
// The sector size of FAT file systems on SD cards.
#define UTIL_SECTOR_SIZE 512u
// The largest piece moved through the staging buffer at once.
#define UTIL_STAGE_SIZE 32768u

void *UTIL_fopen_without_checking(const char *_FileName, const char *_Mode)
{
    FILE *fp = NULL;

    if (_FileName == NULL || _Mode == NULL)
        TerminateOnError("%s() failed: invalid arguments (filename or mode is NULL)\n", __func__);
    else
        fp = fopen(_FileName, _Mode);

    return fp;
}

void *UTIL_fopen(const char *_FileName, const char *_Mode)
{
    FILE *fp = UTIL_fopen_without_checking(_FileName, _Mode);
    if (fp == NULL)
        TerminateOnError("%s() failed: cannot open file '%s' (file not found or permission denied)\n", __func__, _FileName);

    return fp;
}

int UTIL_fseek(void *_Stream, int64_t _Offset, int _Origin)
{
    if (_Stream == NULL)
    {
        TerminateOnError("%s() failed: invalid argument (stream is NULL)\n", __func__);
        return -1;
    }
    else
    {
        PROF_COUNT(PROF_FSEEK, 1);
#if defined(_MSC_VER)
        /* On MSVC use the non-locking CRT variant to avoid FILE* locking overhead.
           Use this only when the caller guarantees single-threaded access or external synchronization. */
        return _fseek_nolock((FILE *)_Stream, _Offset, _Origin);
#else
        /* Portable fallback */
        return fseek((FILE *)_Stream, _Offset, _Origin);
#endif
    }
}

// Read bytes with the platform's fastest stream read.
static size_t ReadPlain(void *buffer, size_t size, FILE *fp)
{
#if defined(_MSC_VER)
    /* On MSVC use the non-locking CRT variant to avoid FILE* locking overhead.
       Use this only when the caller guarantees single-threaded access or external synchronization. */
    return _fread_nolock(buffer, 1, size, fp);
#elif defined(_POSIX_VERSION) || defined(__unix__) || defined(__APPLE__)
    /* On POSIX, fread_unlocked reduces locking overhead in similar scenarios. */
    return fread_unlocked(buffer, 1, size, fp);
#else
    /* Portable fallback */
    return fread(buffer, 1, size, fp);
#endif
}

// Write bytes with the platform's fastest stream write.
static size_t WritePlain(const void *buffer, size_t size, FILE *fp)
{
#if defined(_MSC_VER)
    /* On MSVC use the non-locking CRT variant to avoid FILE* locking overhead.
       Use this only when the caller guarantees single-threaded access or external synchronization. */
    return _fwrite_nolock(buffer, 1, size, fp);
#elif defined(_POSIX_VERSION) || defined(__unix__) || defined(__APPLE__)
    /* On POSIX, fwrite_unlocked reduces locking overhead in similar scenarios. */
    return fwrite_unlocked(buffer, 1, size, fp);
#else
    /* Portable fallback */
    return fwrite(buffer, 1, size, fp);
#endif
}

#if UTIL_SECTOR_IO
// Allocate a staging buffer the SD card driver can transfer to and from directly.
static uint8_t *AllocStage(void)
{
#if defined(ARDUINO_ARCH_ESP32)
    // The P4 transfers into PSRAM by DMA when the buffer is cache-line aligned.
    return (uint8_t *)heap_caps_aligned_alloc(128, UTIL_STAGE_SIZE, MALLOC_CAP_SPIRAM);
#else
    return (uint8_t *)malloc(UTIL_STAGE_SIZE);
#endif
}

// Free a staging buffer from AllocStage.
static void FreeStage(uint8_t *stage)
{
#if defined(ARDUINO_ARCH_ESP32)
    heap_caps_free(stage);
#else
    free(stage);
#endif
}

// The size of the next piece of a transfer at file position pos with left
// bytes to go: up to the next sector boundary, then whole sectors up to the
// stage size, then the rest of the last sector.
static size_t NextPiece(long pos, size_t left)
{
    size_t offset = (size_t)(pos % UTIL_SECTOR_SIZE);

    if (offset != 0)
        return left < UTIL_SECTOR_SIZE - offset ? left : UTIL_SECTOR_SIZE - offset;
    if (left < UTIL_SECTOR_SIZE)
        return left;
    left -= left % UTIL_SECTOR_SIZE;
    return left < UTIL_STAGE_SIZE ? left : UTIL_STAGE_SIZE;
}

/*++
  Note: the P4's SD card driver moves many sectors per command only into or out
  of a cache-line aligned buffer, starting at a sector boundary of the file;
  every other transfer goes one sector per command (1.2 MB/s read instead of
  4.4 MB/s, measured). Stream transfers never meet both conditions, so a large
  transfer goes to the file descriptor in sector-aligned pieces through an
  aligned staging buffer, and the stream is moved to the new position after.
  Each call allocates its own stage, since the audio task reads music through
  UTIL_fread too.
--*/
static size_t ReadSectors(uint8_t *buffer, size_t size, FILE *fp)
{
    long pos = ftell(fp);
    int fd = UTIL_FILENO(fp);
    uint8_t *stage = pos < 0 ? NULL : AllocStage();
    size_t done = 0, want;
    int got;

    // Fall back to the stream when the position or a stage is not available.
    if (stage == NULL || UTIL_LSEEK(fd, pos, SEEK_SET) != pos)
    {
        if (stage != NULL)
            FreeStage(stage);
        return ReadPlain(buffer, size, fp);
    }

    // Read piece by piece until the end or a short read.
    while (done < size)
    {
        want = NextPiece(pos + (long)done, size - done);
        got = (int)UTIL_READ(fd, stage, want);
        if (got <= 0)
            break;
        memcpy(buffer + done, stage, (size_t)got);
        done += (size_t)got;
        if ((size_t)got < want)
            break;
    }

    // Move the stream to where the descriptor stopped.
    fseek(fp, pos + (long)done, SEEK_SET);
    FreeStage(stage);
    return done;
}

// Write like ReadSectors reads, after handing the stream's buffered bytes to the file.
static size_t WriteSectors(const uint8_t *buffer, size_t size, FILE *fp)
{
    long pos = fflush(fp) == 0 ? ftell(fp) : -1;
    int fd = UTIL_FILENO(fp);
    uint8_t *stage = pos < 0 ? NULL : AllocStage();
    size_t done = 0, want;
    int got;

    // Fall back to the stream when the position or a stage is not available.
    if (stage == NULL || UTIL_LSEEK(fd, pos, SEEK_SET) != pos)
    {
        if (stage != NULL)
            FreeStage(stage);
        return WritePlain(buffer, size, fp);
    }

    // Write piece by piece until the end or a short write.
    while (done < size)
    {
        want = NextPiece(pos + (long)done, size - done);
        memcpy(stage, buffer + done, want);
        got = (int)UTIL_WRITE(fd, stage, want);
        if (got <= 0)
            break;
        done += (size_t)got;
        if ((size_t)got < want)
            break;
    }

    // Move the stream to where the descriptor stopped.
    fseek(fp, pos + (long)done, SEEK_SET);
    FreeStage(stage);
    return done;
}
#endif

uint32_t UTIL_fread(void *_Buffer, uint32_t _ElementSize, uint32_t _ElementCount, void *_Stream)
{
    size_t size, done;

    if (_Buffer == NULL || _Stream == NULL)
    {
        TerminateOnError("%s() failed: invalid arguments (buffer or stream is NULL)\n", __func__);
        return 0;
    }

    size = (size_t)_ElementSize * _ElementCount;
    PROF_COUNT(PROF_FREAD, 1);
    PROF_COUNT(PROF_FREAD_BYTES, (uint32_t)size);
    if (size == 0)
        return 0;

    // Read the bytes; like fread, report the complete elements read.
    PROF_BEGIN(read);
#if UTIL_SECTOR_IO
    if (size >= UTIL_SECTOR_IO_MIN)
        done = ReadSectors((uint8_t *)_Buffer, size, (FILE *)_Stream);
    else
#endif
        done = ReadPlain(_Buffer, size, (FILE *)_Stream);
    PROF_END(read, PROF_TIME_READ);
    return (uint32_t)(done / _ElementSize);
}

uint32_t UTIL_fwrite(void *_Buffer, uint32_t _ElementSize, uint32_t _ElementCount, void *_Stream)
{
    size_t size, done;

    if (_Buffer == NULL || _Stream == NULL)
    {
        TerminateOnError("%s() failed: invalid arguments (buffer or stream is NULL)\n", __func__);
        return 0;
    }

    size = (size_t)_ElementSize * _ElementCount;
    PROF_COUNT(PROF_FWRITE, 1);
    PROF_COUNT(PROF_FWRITE_BYTES, (uint32_t)size);
    if (size == 0)
        return 0;

    // Write the bytes; like fwrite, report the complete elements written.
#if UTIL_SECTOR_IO
    if (size >= UTIL_SECTOR_IO_MIN)
        done = WriteSectors((const uint8_t *)_Buffer, size, (FILE *)_Stream);
    else
#endif
        done = WritePlain(_Buffer, size, (FILE *)_Stream);
    return (uint32_t)(done / _ElementSize);
}

void UTIL_fclose(void *fp)
{
    if (fp != NULL)
    {
#if defined(_MSC_VER)
        /* On MSVC use the non-locking CRT variant to avoid FILE* locking overhead.
           Use this only when the caller guarantees single-threaded access or external synchronization. */
        _fclose_nolock((FILE *)fp);
#else
        /* Portable fallback */
        fclose((FILE *)fp);
#endif
    }
}

long UTIL_FileLength(void *fp)
{
    if (fp == NULL)
        TerminateOnError("%s() failed: invalid argument (file pointer is NULL)\n", __func__);

#if defined(_WIN32) || defined(_WIN64)
    /* On Windows, use GetFileSizeEx for efficiency */
    LARGE_INTEGER size;
    if (GetFileSizeEx((HANDLE)_get_osfhandle(_fileno((FILE *)fp)), &size))
    {
        return (long)size.QuadPart;
    }
    else
    {
        TerminateOnError("%s() failed: GetFileSizeEx error\n", __func__);
        return 0;
    }
#else
    /* On POSIX, use fstat for efficiency */
    struct stat st;
    if (fstat(fileno((FILE *)fp), &st) == 0)
    {
        return (long)st.st_size;
    }
    else
    {
        TerminateOnError("%s() failed: fstat error\n", __func__);
        return 0;
    }
#endif
}
