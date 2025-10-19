#include "util.h"
#include <stdio.h>

static char *gFiles_name[Res_Count] = {
    [Res_ABC] = RESOURCE_PATH "/abc.mkf",
    [Res_FBP] = RESOURCE_PATH "/fbp.mkf",
    [Res_MGO] = RESOURCE_PATH "/mgo.mkf",
    [Res_BALL] = RESOURCE_PATH "/ball.mkf",
    [Res_DATA] = RESOURCE_PATH "/data.mkf",
    [Res_F] = RESOURCE_PATH "/f.mkf",
    [Res_FIRE] = RESOURCE_PATH "/fire.mkf",
    [Res_RGM] = RESOURCE_PATH "/rgm.mkf",
    [Res_SOUNDS] = RESOURCE_PATH "/sounds.mkf",
    [Res_PAT] = RESOURCE_PATH "/pat.mkf",
    [Res_MAP] = RESOURCE_PATH "/map.mkf",
    [Res_GOP] = RESOURCE_PATH "/gop.mkf",
    [Res_MUS] = RESOURCE_PATH "/mus.mkf",
    [Res_RNG] = RESOURCE_PATH "/rng.mkf",
};
static unsigned char gFiles_created[Res_Count] = {0};
static FILE *gFiles_fp[Res_Count] = {NULL};

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

void *UTIL_Open_without_checking(const PALRES res, const char *_Mode)
{
    if (gFiles_name[res] == NULL || _Mode == NULL)
        TerminateOnError("%s() failed: invalid arguments (resource name or mode is NULL)\n", __func__);
    else if (gFiles_fp[res] == NULL)
        gFiles_fp[res] = fopen(gFiles_name[res], _Mode);

    if (gFiles_fp[res] != NULL)
    {
        gFiles_created[res]++;
        // fprintf(stdout, "File %s loaded\n", gFiles_name[res]);
    }

    return gFiles_fp[res];
}

void *UTIL_Open(const PALRES res, const char *_Mode)
{
    FILE *fp = UTIL_Open_without_checking(res, _Mode);

    if (fp == NULL)
        TerminateOnError("%s() failed: cannot open resource file '%s' (file not found or access denied)\n", __func__, gFiles_name[res]);

    return fp;
}

unsigned char UTIL_Close(const PALRES res)
{
    if (gFiles_created[res])
        gFiles_created[res]--;

    if ((gFiles_created[res] == 0) && (gFiles_fp[res] != NULL))
    {
        UTIL_fclose(gFiles_fp[res]);
        gFiles_fp[res] = NULL;
    }
    return gFiles_created[res];
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
