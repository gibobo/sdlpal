//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "../audio.h"
#include "../driver.h"
#include "../global.h"
#include "../palcommon.h"
#include "../resource.h"
#include "../util.h"
#include "resampler.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* Maximum number of simultaneous sounds allowed in the queue */
#define MAX_SOUND_QUEUE_DEPTH 4

typedef struct RIFFHeader
{
    unsigned int signature; /* 'RIFF' */
    unsigned int length;    /* Total length minus eight, little-endian */
    unsigned int type;      /* 'WAVE', 'AVI ', ... */
} RIFFHeader;

typedef struct RIFFChunkHeader
{
    unsigned int type;   /* 'fmt ', 'hdrl', 'movi' and so on */
    unsigned int length; /* Total chunk length minus eight, little-endian */
} RIFFChunkHeader;

typedef struct WAVEFormatPCM
{
    unsigned short wFormatTag;    /* format type */
    unsigned char nChannels;      /* number of channels (i.e. mono, stereo, etc.) */
    unsigned int nSamplesPerSec;  /* sample rate */
    unsigned int nAvgBytesPerSec; /* for buffer estimation */
    unsigned short nBlockAlign;   /* block size of data */
    unsigned short wBitsPerSample;
} WAVEFormatPCM;

typedef struct tagWAVESPEC
{
    unsigned int size;
    unsigned int freq;
    unsigned short format;
    unsigned char channels;
    unsigned char align;
} WAVESPEC;

typedef int (*ResampleMixer)(void *[2], const void *, const WAVESPEC *, void *, unsigned int, const void **);

typedef struct tagWAVEDATA
{
    struct tagWAVEDATA *next;
    void *resampler[2]; /* The resampler used for sound data */
    ResampleMixer ResampleMix;
    const void *base;
    const void *current;
    const void *end;
    WAVESPEC spec;
} WAVEDATA;

typedef struct tagSOUNDPLAYER
{
    AUDIOPLAYER_COMMONS;
    WAVEDATA soundlist;
    int cursounds;
    int lastSFX;
} SOUNDPLAYER;

typedef struct tagVOCHEADER
{
    char signature[0x14];       /* "Creative Voice File" */
    unsigned short data_offset; /* little endian */
    unsigned short version;
    unsigned short version_checksum;
} VOCHEADER;

#define RIFF_RIFF (((unsigned int)'R') | (((unsigned int)'I') << 8) | (((unsigned int)'F') << 16) | (((unsigned int)'F') << 24))
#define RIFF_WAVE (((unsigned int)'W') | (((unsigned int)'A') << 8) | (((unsigned int)'V') << 16) | (((unsigned int)'E') << 24))
#define WAVE_fmt  (((unsigned int)'f') | (((unsigned int)'m') << 8) | (((unsigned int)'t') << 16) | (((unsigned int)' ') << 24))
#define WAVE_data (((unsigned int)'d') | (((unsigned int)'a') << 8) | (((unsigned int)'t') << 16) | (((unsigned int)'a') << 24))

static const void *SOUND_LoadWAVEData(const unsigned char *lpData, unsigned int dwLen, WAVESPEC *lpSpec)
/*++
  Purpose:

    Return the WAVE data pointer inside the input buffer.

  Parameters:

    [IN]  lpData - pointer to the buffer of the WAVE file.

    [IN]  dwLen - length of the buffer of the WAVE file.

    [OUT] lpSpec - pointer to the AudioSpec structure, which contains
                    some basic information about the WAVE file.

  Return value:

    Pointer to the WAVE data inside the input buffer, NULL if failed.
--*/
{
    const RIFFHeader *lpRiff = (const RIFFHeader *)lpData;
    const RIFFChunkHeader *lpChunk = NULL;
    const WAVEFormatPCM *lpFormat = NULL;
    const unsigned char *lpWaveData = NULL;
    unsigned int len = 0, type;

    if (dwLen < sizeof(RIFFHeader) || lpRiff->signature != RIFF_RIFF ||
        lpRiff->type != RIFF_WAVE || dwLen < (lpRiff->length + 8))
    {
        return NULL;
    }

    lpChunk = (const RIFFChunkHeader *)(lpRiff + 1);
    dwLen -= sizeof(RIFFHeader);
    while (dwLen >= sizeof(RIFFChunkHeader))
    {
        len = lpChunk->length;
        type = lpChunk->type;
        if (dwLen >= sizeof(RIFFChunkHeader) + len)
            dwLen -= sizeof(RIFFChunkHeader) + len;
        else
            return NULL;

        switch (type)
        {
            case WAVE_fmt:
                lpFormat = (const WAVEFormatPCM *)(lpChunk + 1);
                if (len != sizeof(WAVEFormatPCM) || lpFormat->wFormatTag != 0x0001)
                {
                    return NULL;
                }
                break;
            case WAVE_data:
                lpWaveData = (const unsigned char *)(lpChunk + 1);
                dwLen = 0;
                break;
        }
        lpChunk = (const RIFFChunkHeader *)((const unsigned char *)(lpChunk + 1) + len);
    }

    if (lpFormat == NULL || lpWaveData == NULL)
    {
        return NULL;
    }

    lpSpec->channels = lpFormat->nChannels;
    lpSpec->format = (lpFormat->wBitsPerSample == 16);
    lpSpec->freq = lpFormat->nSamplesPerSec;
    lpSpec->size = len;
    lpSpec->align = (unsigned char)((lpFormat->nChannels * lpFormat->wBitsPerSample) >> 3);

    return lpWaveData;
}

static int SOUND_ResampleMix_Common(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    unsigned int iBufLen,
    const void **llpData,
    bool is16bit,
    unsigned int input_channels,
    unsigned int output_channels)
{
    const unsigned char *src_u8 = (const unsigned char *)lpData;
    const short *src_s16 = (const short *)lpData;
    const void *src_any = lpData;
    PAL_AUDIO_SAMPLE *dst = (PAL_AUDIO_SAMPLE *)lpBuffer;
    const unsigned int bytes_per_sample = is16bit ? sizeof(short) : sizeof(unsigned char);
    const unsigned int bytes_per_frame = input_channels * bytes_per_sample;
    unsigned int frames = bytes_per_frame ? (lpSpec->size / bytes_per_frame) : 0U;
    unsigned int channel_len = (output_channels > 0U) ? (iBufLen / output_channels) : 0U;
    unsigned int total_bytes = 0U;

    while (total_bytes < channel_len && frames > 0U)
    {
        unsigned int to_write = resampler_get_free_count(resampler[0]);
        if (to_write > frames)
            to_write = frames;

        if (is16bit)
        {
            for (unsigned int j = 0; j < to_write; ++j)
            {
                resampler_write_sample(resampler[0], *src_s16++);
                if (input_channels == 2U)
                    resampler_write_sample(resampler[1], *src_s16++);
            }
            src_any = src_s16;
        }
        else
        {
            for (unsigned int j = 0; j < to_write; ++j)
            {
                resampler_write_sample(resampler[0], (*src_u8++ ^ 0x80) << 8);
                if (input_channels == 2U)
                    resampler_write_sample(resampler[1], (*src_u8++ ^ 0x80) << 8);
            }
            src_any = src_u8;
        }

        frames -= to_write;

        while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
        {
            if (output_channels == 1U)
            {
                int mixed;
                if (input_channels == 1U)
                {
                    int sample = resampler_get_sample(resampler[0]) >> 8;
                    mixed = PAL_AudioSampleToMixValue(*dst) + sample;
                    resampler_remove_sample(resampler[0]);
                }
                else
                {
                    int left = resampler_get_sample(resampler[0]) >> 8;
                    int right = resampler_get_sample(resampler[1]) >> 8;
                    mixed = PAL_AudioSampleToMixValue(*dst) + ((left + right) >> 1);
                    resampler_remove_sample(resampler[0]);
                    resampler_remove_sample(resampler[1]);
                }
                *dst++ = PAL_AudioMixValueToSample(mixed);
                total_bytes += PAL_AUDIO_BYTES_PER_SAMPLE;
            }
            else
            {
                int left_sample = resampler_get_sample(resampler[0]) >> 8;
                int mixed_left = PAL_AudioSampleToMixValue(dst[0]) + left_sample;
                PAL_AUDIO_SAMPLE output_left = PAL_AudioMixValueToSample(mixed_left);

                if (input_channels == 1U)
                {
                    dst[0] = output_left;
                    dst[1] = output_left;
                    resampler_remove_sample(resampler[0]);
                }
                else
                {
                    int right_sample = resampler_get_sample(resampler[1]) >> 8;
                    int mixed_right = PAL_AudioSampleToMixValue(dst[1]) + right_sample;
                    dst[0] = output_left;
                    dst[1] = PAL_AudioMixValueToSample(mixed_right);
                    resampler_remove_sample(resampler[0]);
                    resampler_remove_sample(resampler[1]);
                }

                dst += 2;
                total_bytes += PAL_AUDIO_BYTES_PER_SAMPLE;
            }
        }
    }

    if (llpData)
        *llpData = src_any;

    return total_bytes;
}

static int SOUND_ResampleMix_U8_Mono_Mono(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    unsigned int iBufLen,
    const void **llpData)
/*++
  Purpose:

    Resample 8-bit unsigned mono PCM data into 16-bit signed (native-endian) mono PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    return SOUND_ResampleMix_Common(resampler, lpData, lpSpec, lpBuffer, iBufLen, llpData, false, 1U, 1U);
}

static int SOUND_ResampleMix_U8_Mono_Stereo(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    unsigned int iBufLen,
    const void **llpData)
/*++
  Purpose:

    Resample 8-bit unsigned mono PCM data into 16-bit signed (native-endian) stereo PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    return SOUND_ResampleMix_Common(resampler, lpData, lpSpec, lpBuffer, iBufLen, llpData, false, 1U, 2U);
}

static int SOUND_ResampleMix_U8_Stereo_Mono(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    unsigned int iBufLen,
    const void **llpData)
/*++
  Purpose:

    Resample 8-bit unsigned stereo PCM data into 16-bit signed (native-endian) mono PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    return SOUND_ResampleMix_Common(resampler, lpData, lpSpec, lpBuffer, iBufLen, llpData, false, 2U, 1U);
}

static int SOUND_ResampleMix_U8_Stereo_Stereo(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    unsigned int iBufLen,
    const void **llpData)
/*++
  Purpose:

    Resample 8-bit unsigned stereo PCM data into 16-bit signed (native-endian) stereo PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    return SOUND_ResampleMix_Common(resampler, lpData, lpSpec, lpBuffer, iBufLen, llpData, false, 2U, 2U);
}

static int SOUND_ResampleMix_S16_Mono_Mono(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    unsigned int iBufLen,
    const void **llpData)
/*++
  Purpose:

    Resample 16-bit signed (little-endian) mono PCM data into 16-bit signed (native-endian) mono PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    return SOUND_ResampleMix_Common(resampler, lpData, lpSpec, lpBuffer, iBufLen, llpData, true, 1U, 1U);
}

static int SOUND_ResampleMix_S16_Mono_Stereo(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    unsigned int iBufLen,
    const void **llpData)
/*++
  Purpose:

    Resample 16-bit signed (little-endian) mono PCM data into 16-bit signed (native-endian) stereo PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    return SOUND_ResampleMix_Common(resampler, lpData, lpSpec, lpBuffer, iBufLen, llpData, true, 1U, 2U);
}

static int SOUND_ResampleMix_S16_Stereo_Mono(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    unsigned int iBufLen,
    const void **llpData)
/*++
  Purpose:

    Resample 16-bit signed (little-endian) stereo PCM data into 16-bit signed (native-endian) mono PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    return SOUND_ResampleMix_Common(resampler, lpData, lpSpec, lpBuffer, iBufLen, llpData, true, 2U, 1U);
}

static int SOUND_ResampleMix_S16_Stereo_Stereo(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    unsigned int iBufLen,
    const void **llpData)
/*++
  Purpose:

    Resample 16-bit signed (little-endian) stereo PCM data into 16-bit signed (native-endian) stereo PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    return SOUND_ResampleMix_Common(resampler, lpData, lpSpec, lpBuffer, iBufLen, llpData, true, 2U, 2U);
}

static int SOUND_Play(
    void *object,
    int iSoundNum,
    unsigned char fLoop,
    float flFadeTime)
/*++
  Purpose:

    Play a sound in voc.mkf/sounds.mkf file.

  Parameters:

    [IN]  object - Pointer to the SOUNDPLAYER instance.
    [IN]  iSoundNum - number of the sound; the absolute value is used.
    [IN]  fLoop - Not used, should be zero.
    [IN]  flFadeTime - Not used, should be zero.

  Return value:

    None.

--*/
{
    SOUNDPLAYER *player = (SOUNDPLAYER *)object;
    WAVESPEC wavespec;
    ResampleMixer mixer = NULL;
    WAVEDATA *cursnd;
    unsigned char *buf = NULL;
    unsigned int buf_size = RES_MKFCreateChunk(&buf, iSoundNum, Res_SOUNDS);
    const void *snddata;
    unsigned char i;
    unsigned int len;

    (void)fLoop;
    (void)flFadeTime;

    // Check for NULL pointer.
    if (player == NULL)
    {
        return false;
    }

    if (player->lastSFX == iSoundNum)
        return false;

    player->lastSFX = iSoundNum;

    len = RES_MKFReadChunk(buf, buf_size, iSoundNum, Res_SOUNDS);
    if (len <= 0)
    {
        return false;
    }

    snddata = SOUND_LoadWAVEData(buf, len, &wavespec);
    if (snddata == NULL)
    {
        UTIL_free(buf);
        return false;
    }
#if PAL_AUDIO_CHANNEL_COUNT == 1U
    if (wavespec.channels == 1)
        mixer = (wavespec.format) ? SOUND_ResampleMix_S16_Mono_Mono : SOUND_ResampleMix_U8_Mono_Mono;
    if (wavespec.channels == 2)
        mixer = (wavespec.format) ? SOUND_ResampleMix_S16_Stereo_Mono : SOUND_ResampleMix_U8_Stereo_Mono;
#elif PAL_AUDIO_CHANNEL_COUNT == 2U
    if (wavespec.channels == 1)
        mixer = (wavespec.format) ? SOUND_ResampleMix_S16_Mono_Stereo : SOUND_ResampleMix_U8_Mono_Stereo;
    if (wavespec.channels == 2)
        mixer = (wavespec.format) ? SOUND_ResampleMix_S16_Stereo_Stereo : SOUND_ResampleMix_U8_Stereo_Stereo;
#endif
    if (mixer == NULL)
    {
        UTIL_free(buf);
        return false;
    }

    DRIVER_Audio_Lock();

    cursnd = &player->soundlist;
    int queue_depth = 0;
    while (cursnd->next && cursnd->base)
    {
        cursnd = cursnd->next;
        queue_depth++;
    }

    // Limit queue depth to prevent memory accumulation
    if (queue_depth >= MAX_SOUND_QUEUE_DEPTH)
    {
        DRIVER_Audio_Unlock();
        UTIL_free(buf);
        return false;
    }

    if (cursnd->base)
    {
        WAVEDATA *obj = (WAVEDATA *)UTIL_malloc(sizeof(WAVEDATA));
        cursnd->next = obj;
        cursnd = cursnd->next;
    }

    for (i = 0; i < wavespec.channels; i++)
    {
        if (!cursnd->resampler[i])
            cursnd->resampler[i] = resampler_create();
        else
            resampler_clear(cursnd->resampler[i]);
        resampler_set_quality(cursnd->resampler[i], ((wavespec.freq % PAL_AUDIO_SAMPLING_RATE) == 0 || (PAL_AUDIO_SAMPLING_RATE % wavespec.freq) == 0) ? RESAMPLER_QUALITY_MIN : RESAMPLER_QUALITY_MAX);
        resampler_set_rate(cursnd->resampler[i], (double)wavespec.freq / (double)PAL_AUDIO_SAMPLING_RATE);
    }

    cursnd->base = buf;
    cursnd->current = snddata;
    cursnd->end = (const unsigned char *)snddata + wavespec.size;
    cursnd->spec = wavespec;
    cursnd->ResampleMix = mixer;
    player->cursounds++;

    DRIVER_Audio_Unlock();

    return true;
}

void SOUND_Shutdown(void *object)
/*++
  Purpose:

    Shutdown the sound subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    SOUNDPLAYER *player = (SOUNDPLAYER *)object;
    if (player)
    {
        WAVEDATA *cursnd = &player->soundlist;
        do
        {
            if (cursnd->resampler[0])
                resampler_delete(cursnd->resampler[0]);
            if (cursnd->resampler[1])
                resampler_delete(cursnd->resampler[1]);
            if (cursnd->base)
                UTIL_free((void *)cursnd->base);
        } while ((cursnd = cursnd->next) != NULL);
        cursnd = player->soundlist.next;
        while (cursnd)
        {
            WAVEDATA *old = cursnd;
            cursnd = cursnd->next;
            UTIL_free(old);
        }
    }
    resampler_deinit();
}

static void SOUND_FillBuffer(
    void *object,
    unsigned char *stream,
    unsigned int len)
/*++
  Purpose:

    Fill the background music into the sound buffer. Called by the SDL sound
    callback function only (audio.c: AUDIO_FillBuffer).

  Parameters:

    [OUT] stream - pointer to the stream buffer.

    [IN]  len - Length of the buffer.

  Return value:

    None.

--*/
{
    SOUNDPLAYER *player = (SOUNDPLAYER *)object;
    if (player)
    {
        WAVEDATA *cursnd = &player->soundlist;
        int sounds = 0;
        do
        {
            if (cursnd->base)
            {
                cursnd->ResampleMix(cursnd->resampler, cursnd->current, &cursnd->spec, stream, len, &cursnd->current);
                cursnd->spec.size = (int)((const unsigned char *)cursnd->end - (const unsigned char *)cursnd->current);
                if (cursnd->spec.size < cursnd->spec.align)
                {
                    UTIL_free((void *)cursnd->base);
                    cursnd->base = cursnd->current = cursnd->end = NULL;
                    player->cursounds--;
                    player->lastSFX = 0;
                }
                else
                    sounds++;
            }
        } while ((cursnd = cursnd->next) && sounds < player->cursounds);
    }
}

AUDIOPLAYER *SOUND_Init(void)
/*++
  Purpose:

    Initialize the sound subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    // Initialize the resampler module
    resampler_init();

    SOUNDPLAYER *player = (SOUNDPLAYER *)UTIL_malloc(sizeof(SOUNDPLAYER));
    player->Play = SOUND_Play;
    player->FillBuffer = SOUND_FillBuffer;
    player->Shutdown = SOUND_Shutdown;
    player->soundlist.resampler[0] = resampler_create();
    player->soundlist.resampler[1] = resampler_create();
    player->cursounds = 0;
    return (AUDIOPLAYER *)player;
}
