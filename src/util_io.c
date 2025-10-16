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
    [Res_SSS] = RESOURCE_PATH "/sss.mkf",
    [Res_SOUNDS] = RESOURCE_PATH "/sounds.mkf",
    [Res_PAT] = RESOURCE_PATH "/pat.mkf",
    [Res_MAP] = RESOURCE_PATH "/map.mkf",
    [Res_GOP] = RESOURCE_PATH "/gop.mkf",
    [Res_MUS] = RESOURCE_PATH "/mus.mkf",
    [Res_RNG] = RESOURCE_PATH "/rng.mkf",
    [Save_1] = RESOURCE_PATH "/1.rpg",
    [Save_2] = RESOURCE_PATH "/2.rpg",
    [Save_3] = RESOURCE_PATH "/3.rpg",
    [Save_4] = RESOURCE_PATH "/4.rpg",
    [Save_5] = RESOURCE_PATH "/5.rpg",
    [Cache_Word_2B] = CACHES_PATH "/word_2b.bin",
    [Cache_Word_4B] = CACHES_PATH "/word_4b.bin",
    [Cache_WordLen] = CACHES_PATH "/word_len.bin",
    [Cache_Msg_2B] = CACHES_PATH "/msg_2b.bin",
    [Cache_Msg_4B] = CACHES_PATH "/msg_4b.bin",
    [Cache_MsgLen] = CACHES_PATH "/msg_len.bin",
    [Cache_Font] = CACHES_PATH "/unicode_font.bin",
    [Cache_FontSize] = CACHES_PATH "/unicode_font_size.bin",
};
static unsigned char gFiles_created[Res_Count] = {0};
static FILE *gFiles_fp[Res_Count] = {NULL};

void *UTIL_fopen(const char *_FileName, const char *_Mode)
{
    FILE *fp = NULL;

    if (_FileName == NULL || _Mode == NULL)
        TerminateOnError("%s() called with invalid parameters\n", __func__);
    else
        fp = fopen(_FileName, _Mode);

    if (fp == NULL)
        TerminateOnError("%s() open %s returns a null pointer\n", __func__, _FileName);
    fprintf(stdout, "File %s loaded\n", _FileName);
    return fp;
}

void *UTIL_fopen_without_checking(const char *_FileName, const char *_Mode)
{
    FILE *fp = NULL;

    if (_FileName == NULL || _Mode == NULL)
        TerminateOnError("%s() called with invalid parameters\n", __func__);
    else
        fp = fopen(_FileName, _Mode);

    if (fp != NULL)
        fprintf(stdout, "File %s loaded\n", _FileName);

    return fp;
}

int UTIL_fseek(void *_Stream, long _Offset, int _Origin)
{
    if (_Stream == NULL)
        TerminateOnError("%s() called with invalid parameters\n", __func__);
    else
        return fseek((FILE *)_Stream, _Offset, _Origin);

    return 0;
}

unsigned int UTIL_fread(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream)
{
    if (_Buffer == NULL || _Stream == NULL)
        TerminateOnError("%s() called with invalid parameters\n", __func__);
    else
        return (unsigned int)fread(_Buffer, _ElementSize, _ElementCount, (FILE *)_Stream);

    return 0;
}

unsigned int UTIL_fwrite(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream)
{
    if (_Buffer == NULL || _Stream == NULL)
        TerminateOnError("%s() called with invalid parameters\n", __func__);
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
        TerminateOnError("%s() called with invalid parameters\n", __func__);
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
    if (UTIL_Open_without_checking(res, _Mode) == NULL)
        TerminateOnError("%s() open %s failed\n", __func__, gFiles_name[res]);

    return gFiles_fp[res];
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

long UTIL_flength(void *fp)
{
    long old_pos = ftell((FILE *)fp);

    if (old_pos == -1)
        return -1;

    if (UTIL_fseek((FILE *)fp, 0, SEEK_END) == -1)
        return -1;

    long length = ftell((FILE *)fp);
    UTIL_fseek((FILE *)fp, old_pos, SEEK_SET);
    return length;
}
