#include "util.h"
#include <stdio.h>

struct
{
    char *name;
    void *fp;
    unsigned char created;
} gFiles[] = {
    [Res_ABC] = {RESOURCE_PATH "/abc.mkf", NULL, 0},
    [Res_FBP] = {RESOURCE_PATH "/fbp.mkf", NULL, 0},
    [Res_MGO] = {RESOURCE_PATH "/mgo.mkf", NULL, 0},
    [Res_BALL] = {RESOURCE_PATH "/ball.mkf", NULL, 0},
    [Res_DATA] = {RESOURCE_PATH "/data.mkf", NULL, 0},
    [Res_F] = {RESOURCE_PATH "/f.mkf", NULL, 0},
    [Res_FIRE] = {RESOURCE_PATH "/fire.mkf", NULL, 0},
    [Res_RGM] = {RESOURCE_PATH "/rgm.mkf", NULL, 0},
    [Res_SSS] = {RESOURCE_PATH "/sss.mkf", NULL, 0},
    [Res_SOUNDS] = {RESOURCE_PATH "/sounds.mkf", NULL, 0},
    [Res_PAT] = {RESOURCE_PATH "/pat.mkf", NULL, 0},
    [Res_MAP] = {RESOURCE_PATH "/map.mkf", NULL, 0},
    [Res_GOP] = {RESOURCE_PATH "/gop.mkf", NULL, 0},
    [Res_MUS] = {RESOURCE_PATH "/mus.mkf", NULL, 0},
    [Res_RNG] = {RESOURCE_PATH "/rng.mkf", NULL, 0},
    [Save_1] = {RESOURCE_PATH "/1.rpg", NULL, 0},
    [Save_2] = {RESOURCE_PATH "/2.rpg", NULL, 0},
    [Save_3] = {RESOURCE_PATH "/3.rpg", NULL, 0},
    [Save_4] = {RESOURCE_PATH "/4.rpg", NULL, 0},
    [Save_5] = {RESOURCE_PATH "/5.rpg", NULL, 0},
    [Cache_Word_2B] = {CACHES_PATH "/word_2b.bin", NULL, 0},
    [Cache_Word_4B] = {CACHES_PATH "/word_4b.bin", NULL, 0},
    [Cache_WordLen] = {CACHES_PATH "/word_len.bin", NULL, 0},
    [Cache_Msg_2B] = {CACHES_PATH "/msg_2b.bin", NULL, 0},
    [Cache_Msg_4B] = {CACHES_PATH "/msg_4b.bin", NULL, 0},
    [Cache_MsgLen] = {CACHES_PATH "/msg_len.bin", NULL, 0},
    [Cache_Font] = {CACHES_PATH "/unicode_font.bin", NULL, 0},
    [Cache_FontSize] = {CACHES_PATH "/unicode_font_size.bin", NULL, 0},
};

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
    if (gFiles[res].name == NULL || _Mode == NULL)
        TerminateOnError("%s() called with invalid parameters\n", __func__);
    else if (gFiles[res].fp == NULL)
        gFiles[res].fp = fopen(gFiles[res].name, _Mode);

    if (gFiles[res].fp != NULL)
    {
        gFiles[res].created++;
        // fprintf(stdout, "File %s loaded\n", gFiles[res].name);
    }

    return gFiles[res].fp;
}

void *UTIL_Open(const PALRES res, const char *_Mode)
{
    if(UTIL_Open_without_checking(res, _Mode) == NULL)
        TerminateOnError("%s() open %s failed\n", __func__, gFiles[res].name);

    return gFiles[res].fp;
}

unsigned char UTIL_Close(const PALRES res)
{
    if (gFiles[res].created)
        gFiles[res].created--;

    if ((gFiles[res].created == 0) && (gFiles[res].fp != NULL))
    {
        UTIL_fclose(gFiles[res].fp);
        gFiles[res].fp = NULL;
    }
    return gFiles[res].created;
}

long flength(void *fp)
{
    long old_pos = ftell((FILE *)fp);

    if (old_pos == -1)
        return -1;

    if (UTIL_fseek(fp, 0, SEEK_END) == -1)
        return -1;

    long length = ftell((FILE *)fp);
    UTIL_fseek(fp, old_pos, SEEK_SET);
    return length;
}
