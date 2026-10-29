#include "util.h"
#include <stdio.h>
#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif

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

uint32_t UTIL_fread(void *_Buffer, uint32_t _ElementSize, uint32_t _ElementCount, void *_Stream)
{
    if (_Buffer == NULL || _Stream == NULL)
    {
        TerminateOnError("%s() failed: invalid arguments (buffer or stream is NULL)\n", __func__);
        return 0;
    }

    /* Match fread signature types to avoid unnecessary casts and allow optimized CRT calls */
    size_t elemSize = (size_t)_ElementSize;
    size_t elemCount = (size_t)_ElementCount;

#if defined(_MSC_VER)
    /* On MSVC use the non-locking CRT variant to avoid FILE* locking overhead.
       Use this only when the caller guarantees single-threaded access or external synchronization. */
    return (uint32_t)_fread_nolock(_Buffer, elemSize, elemCount, (FILE *)_Stream);
#elif defined(_POSIX_VERSION) || defined(__unix__) || defined(__APPLE__)
    /* On POSIX, fread_unlocked reduces locking overhead in similar scenarios. */
    return (uint32_t)fread_unlocked(_Buffer, elemSize, elemCount, (FILE *)_Stream);
#else
    /* Portable fallback */
    return (uint32_t)fread(_Buffer, elemSize, elemCount, (FILE *)_Stream);
#endif
}

uint32_t UTIL_fwrite(void *_Buffer, uint32_t _ElementSize, uint32_t _ElementCount, void *_Stream)
{
    if (_Buffer == NULL || _Stream == NULL)
        TerminateOnError("%s() failed: invalid arguments (buffer or stream is NULL)\n", __func__);
    else
    {
        /* Match fwrite signature types to avoid unnecessary casts and allow optimized CRT calls */
        size_t elemSize = (size_t)_ElementSize;
        size_t elemCount = (size_t)_ElementCount;

#if defined(_MSC_VER)
        /* On MSVC use the non-locking CRT variant to avoid FILE* locking overhead.
           Use this only when the caller guarantees single-threaded access or external synchronization. */
        return (uint32_t)_fwrite_nolock(_Buffer, elemSize, elemCount, (FILE *)_Stream);
#elif defined(_POSIX_VERSION) || defined(__unix__) || defined(__APPLE__)
        /* On POSIX, fwrite_unlocked reduces locking overhead in similar scenarios. */
        return (uint32_t)fwrite_unlocked(_Buffer, elemSize, elemCount, (FILE *)_Stream);
#else
        /* Portable fallback */
        return (uint32_t)fwrite(_Buffer, elemSize, elemCount, (FILE *)_Stream);
#endif
    }

    return 0;
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
