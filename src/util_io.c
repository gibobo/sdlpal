#include "util.h"
#include <stdio.h>

void *UTIL_fopen_without_checking(const char *_FileName, const char *_Mode)
{
    FILE *fp = NULL;

    if (_FileName == NULL || _Mode == NULL)
        TerminateOnError("%s() failed: invalid arguments (filename or mode is NULL)\n", __func__);
    else
        fp = fopen(_FileName, _Mode);

    if (fp != NULL)
        fprintf(stdout, "File %s loaded\n", _FileName);

    return fp;
}

void *UTIL_fopen(const char *_FileName, const char *_Mode)
{
    FILE *fp = UTIL_fopen_without_checking(_FileName, _Mode);
    if (fp == NULL)
        TerminateOnError("%s() failed: cannot open file '%s' (file not found or permission denied)\n", __func__, _FileName);

    return fp;
}

int UTIL_fseek(void *_Stream, long _Offset, int _Origin)
{
    if (_Stream == NULL)
        TerminateOnError("%s() failed: invalid argument (stream is NULL)\n", __func__);
    else
        return fseek((FILE *)_Stream, _Offset, _Origin);

    return 0;
}

unsigned int UTIL_fread(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream)
{
    if (_Buffer == NULL || _Stream == NULL)
        TerminateOnError("%s() failed: invalid arguments (buffer or stream is NULL)\n", __func__);
    else
        return (unsigned int)fread(_Buffer, _ElementSize, _ElementCount, (FILE *)_Stream);

    return 0;
}

unsigned int UTIL_fwrite(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream)
{
    if (_Buffer == NULL || _Stream == NULL)
        TerminateOnError("%s() failed: invalid arguments (buffer or stream is NULL)\n", __func__);
    else
        return (unsigned int)fwrite(_Buffer, _ElementSize, _ElementCount, (FILE *)_Stream);

    return 0;
}

void UTIL_fclose(void *fp)
{
    if (fp != NULL)
        fclose((FILE *)fp);
}

long UTIL_FileLength(void *fp)
{
    if (fp == NULL)
        TerminateOnError("%s() failed: invalid argument (file pointer is NULL)\n", __func__);

    long old_pos = ftell((FILE *)fp);

    if (old_pos == -1)
        TerminateOnError("%s() failed: ftell() error - cannot get current file position\n", __func__);

    if (UTIL_fseek((FILE *)fp, 0, SEEK_END) == -1)
        TerminateOnError("%s() failed: fseek() error - cannot seek to end of file\n", __func__);

    long length = ftell((FILE *)fp); // Get the file length
    if (length == -1)
        TerminateOnError("%s() failed: ftell() error - cannot get file length\n", __func__);

    if (UTIL_fseek((FILE *)fp, old_pos, SEEK_SET) == -1)
        TerminateOnError("%s() failed: fseek() error - cannot restore file position\n", __func__);

    return length;
}
