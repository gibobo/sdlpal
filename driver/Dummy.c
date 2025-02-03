#include "driver.h"
#include "input.h"
#include "util.h"

#ifndef NULL
#define NULL 0
#endif

int DRIVER_Audio(void)
{
    return 0;
}

void DRIVER_DeInit_Audio(void)
{
}

int DRIVER_Init(void)
{
    return 0;
}

void DRIVER_DeInit(void)
{
}

void DRIVER_FrameShow(unsigned char *frame_rgb)
{
}

void DRIVER_FrameResize(unsigned int width, unsigned int height)
{
}

int DRIVER_ProcessEvent(void)
{
    return 0;
}

void *DRIVER_fopen(const char *_FileName, const char *_Mode)
{
    if (_FileName == NULL || _Mode == NULL)
        TerminateOnError("DRIVER_fopen() called with invalid parameters\n");

    return NULL; // fopen(_FileName, _Mode);
}

int DRIVER_fseek(void *_Stream, long _Offset, int _Origin)
{
    if (_Stream == NULL)
        TerminateOnError("DRIVER_fseek() called with invalid parameters\n");

    return 0; // fseek(_Stream, _Offset, _Origin);
}

unsigned int DRIVER_fread(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream)
{
    if (_Buffer == NULL || _Stream == NULL)
        TerminateOnError("DRIVER_fread() called with invalid parameters\n");

    return 0; // (unsigned int)fread(_Buffer, _ElementSize, _ElementCount, _Stream);
}

unsigned int DRIVER_fwrite(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream)
{
    if (_Buffer == NULL || _Stream == NULL)
        TerminateOnError("DRIVER_fread() called with invalid parameters\n");

    return 0; // (unsigned int)fwrite(_Buffer, _ElementSize, _ElementCount, _Stream);
}

void DRIVER_fclose(void *fp)
{
    if (fp != NULL)
    {
        // fclose(fp);
    }
}
