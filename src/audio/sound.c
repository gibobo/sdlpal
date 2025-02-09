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

#include "audio.h"
#include "common.h"
#include "driver.h"
#include "global.h"
#include "palcommon.h"
#include "resampler.h"
#include "util.h"
#include <stdint.h>

typedef struct RIFFHeader {
  uint32_t signature; /* 'RIFF' */
  uint32_t length;    /* Total length minus eight, little-endian */
  uint32_t type;      /* 'WAVE', 'AVI ', ... */
} RIFFHeader;

typedef struct RIFFChunkHeader {
  uint32_t type;   /* 'fmt ', 'hdrl', 'movi' and so on */
  uint32_t length; /* Total chunk length minus eight, little-endian */
} RIFFChunkHeader;

typedef struct WAVEFormatPCM {
  uint16_t wFormatTag;      /* format type */
  uint8_t nChannels;        /* number of channels (i.e. mono, stereo, etc.) */
  uint32_t nSamplesPerSec;  /* sample rate */
  uint32_t nAvgBytesPerSec; /* for buffer estimation */
  uint16_t nBlockAlign;     /* block size of data */
  uint16_t wBitsPerSample;
} WAVEFormatPCM;

typedef struct tagWAVESPEC {
  int size;
  int freq;
  uint16_t format;
  uint8_t channels;
  uint8_t align;
} WAVESPEC;

typedef int (*ResampleMixer)(void *[2], const void *, const WAVESPEC *, void *, int, const void **);

typedef struct tagWAVEDATA {
    struct tagWAVEDATA *next;
    void *resampler[2]; /* The resampler used for sound data */
    ResampleMixer ResampleMix;
    const void *base;
    const void *current;
    const void *end;
    WAVESPEC spec;
} WAVEDATA;

typedef struct tagSOUNDPLAYER {
    AUDIOPLAYER_COMMONS;
    FILE *mkf;             /* File pointer to the MKF file */
    WAVEDATA soundlist;
    int cursounds;
    int lastSFX;
} SOUNDPLAYER;

typedef struct tagVOCHEADER {
  char signature[0x14];       /* "Creative Voice File\x1A" */
  unsigned short data_offset; /* little endian */
  unsigned short version;
  unsigned short version_checksum;
} VOCHEADER;

#define RIFF_RIFF (((uint32_t)'R') | (((uint32_t)'I') << 8) | (((uint32_t)'F') << 16) | (((uint32_t)'F') << 24))
#define RIFF_WAVE (((uint32_t)'W') | (((uint32_t)'A') << 8) | (((uint32_t)'V') << 16) | (((uint32_t)'E') << 24))
#define WAVE_fmt  (((uint32_t)'f') | (((uint32_t)'m') << 8) | (((uint32_t)'t') << 16) | (((uint32_t)' ') << 24))
#define WAVE_data (((uint32_t)'d') | (((uint32_t)'a') << 8) | (((uint32_t)'t') << 16) | (((uint32_t)'a') << 24))

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
    const uint8_t *lpWaveData = NULL;
    uint32_t len, type;

    if (dwLen < sizeof(RIFFHeader) || lpRiff->signature != RIFF_RIFF ||
        lpRiff->type != RIFF_WAVE || dwLen < (lpRiff->length + 8)) {
        return NULL;
    }

    lpChunk = (const RIFFChunkHeader *)(lpRiff + 1);
    dwLen -= sizeof(RIFFHeader);
    while (dwLen >= sizeof(RIFFChunkHeader)) {
      len = lpChunk->length;
      type = lpChunk->type;
      if (dwLen >= sizeof(RIFFChunkHeader) + len)
        dwLen -= sizeof(RIFFChunkHeader) + len;
      else
        return NULL;

      switch (type) {
      case WAVE_fmt:
        lpFormat = (const WAVEFormatPCM *)(lpChunk + 1);
        if (len != sizeof(WAVEFormatPCM) || lpFormat->wFormatTag != 0x0001) {
          return NULL;
        }
        break;
      case WAVE_data:
        lpWaveData = (const uint8_t *)(lpChunk + 1);
        dwLen = 0;
        break;
      }
      lpChunk = (const RIFFChunkHeader *)((const uint8_t *)(lpChunk + 1) + len);
    }

    if (lpFormat == NULL || lpWaveData == NULL) {
        return NULL;
    }

    lpSpec->channels = lpFormat->nChannels;
    lpSpec->format = (lpFormat->wBitsPerSample == 16);
    lpSpec->freq = lpFormat->nSamplesPerSec;
    lpSpec->size = len;
    lpSpec->align = (lpFormat->nChannels * lpFormat->wBitsPerSample) >> 3;

    return lpWaveData;
}

static int SOUND_ResampleMix_U8_Mono_Mono(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    int iBufLen,
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
  int src_samples = lpSpec->size;
  const uint8_t *src = (const uint8_t *)lpData;
  short *dst = (short *)lpBuffer;
  int channel_len = iBufLen, total_bytes = 0;

  while (total_bytes < channel_len && src_samples > 0) {
    int j, to_write = resampler_get_free_count(resampler[0]);
    if (to_write > src_samples)
      to_write = src_samples;
    for (j = 0; j < to_write; j++)
      resampler_write_sample(resampler[0], (*src++ ^ 0x80) << 8);
    src_samples -= to_write;
    while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0) {
      int sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
      *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
      total_bytes += sizeof(short);
      resampler_remove_sample(resampler[0]);
    }
  }

  if (llpData)
    *llpData = src;
  return total_bytes;
}

static int SOUND_ResampleMix_U8_Mono_Stereo(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    int iBufLen,
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
  int src_samples = lpSpec->size;
  const uint8_t *src = (const uint8_t *)lpData;
  short *dst = (short *)lpBuffer;
  int channel_len = iBufLen >> 1, total_bytes = 0;

  while (total_bytes < channel_len && src_samples > 0) {
    int j, to_write = resampler_get_free_count(resampler[0]);
    if (to_write > src_samples)
      to_write = src_samples;
    for (j = 0; j < to_write; j++)
      resampler_write_sample(resampler[0], (*src++ ^ 0x80) << 8);
    src_samples -= to_write;
    while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0) {
      int sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
      dst[0] = dst[1] = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
      total_bytes += sizeof(short);
      dst += 2;
      resampler_remove_sample(resampler[0]);
    }
  }

  if (llpData)
    *llpData = src;
  return total_bytes;
}

static int SOUND_ResampleMix_U8_Stereo_Mono(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    int iBufLen,
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
  int src_samples = lpSpec->size >> 1;
  const uint8_t *src = (const uint8_t *)lpData;
  short *dst = (short *)lpBuffer;
  int channel_len = iBufLen, total_bytes = 0;

  while (total_bytes < channel_len && src_samples > 0) {
    int j, to_write = resampler_get_free_count(resampler[0]);
    if (to_write > src_samples)
      to_write = src_samples;
    for (j = 0; j < to_write; j++) {
      resampler_write_sample(resampler[0], (*src++ ^ 0x80) << 8);
      resampler_write_sample(resampler[1], (*src++ ^ 0x80) << 8);
    }
    src_samples -= to_write;
    while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0) {
      int sample = (((resampler_get_sample(resampler[0]) >> 8) + (resampler_get_sample(resampler[1]) >> 8)) >> 1) + *dst;
      *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
      total_bytes += sizeof(short);
      resampler_remove_sample(resampler[0]);
      resampler_remove_sample(resampler[1]);
    }
  }

  if (llpData)
    *llpData = src;
  return total_bytes;
}

static int SOUND_ResampleMix_U8_Stereo_Stereo(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    int iBufLen,
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
  int src_samples = lpSpec->size >> 1;
  const uint8_t *src = (const uint8_t *)lpData;
  short *dst = (short *)lpBuffer;
  int channel_len = iBufLen >> 1, total_bytes = 0;

  while (total_bytes < channel_len && src_samples > 0) {
    int j, to_write = resampler_get_free_count(resampler[0]);
    if (to_write > src_samples)
      to_write = src_samples;
    for (j = 0; j < to_write; j++) {
      resampler_write_sample(resampler[0], (*src++ ^ 0x80) << 8);
      resampler_write_sample(resampler[1], (*src++ ^ 0x80) << 8);
    }
    src_samples -= to_write;
    while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0) {
      int sample;
      sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
      *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
      sample = (resampler_get_sample(resampler[1]) >> 8) + *dst;
      *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
      total_bytes += sizeof(short);
      resampler_remove_sample(resampler[0]);
      resampler_remove_sample(resampler[1]);
    }
  }

  if (llpData)
    *llpData = src;
  return total_bytes;
}

static int SOUND_ResampleMix_S16_Mono_Mono(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    int iBufLen,
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
  int src_samples = lpSpec->size >> 1;
  const short *src = (const short *)lpData;
  short *dst = (short *)lpBuffer;
  int channel_len = iBufLen, total_bytes = 0;

  while (total_bytes < channel_len && src_samples > 0) {
    int j, to_write = resampler_get_free_count(resampler[0]);
    if (to_write > src_samples)
      to_write = src_samples;
    for (j = 0; j < to_write; j++)
      resampler_write_sample(resampler[0], *src++);
    src_samples -= to_write;
    while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0) {
      int sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
      *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
      total_bytes += sizeof(short);
      resampler_remove_sample(resampler[0]);
    }
  }

  if (llpData)
    *llpData = src;
  return total_bytes;
}

static int SOUND_ResampleMix_S16_Mono_Stereo(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    int iBufLen,
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
  int src_samples = lpSpec->size >> 1;
  const short *src = (const short *)lpData;
  short *dst = (short *)lpBuffer;
  int channel_len = iBufLen >> 1, total_bytes = 0;

  while (total_bytes < channel_len && src_samples > 0) {
    int j, to_write = resampler_get_free_count(resampler[0]);
    if (to_write > src_samples)
      to_write = src_samples;
    for (j = 0; j < to_write; j++)
      resampler_write_sample(resampler[0], *src++);
    src_samples -= to_write;
    while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0) {
      int sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
      dst[0] = dst[1] = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
      total_bytes += sizeof(short);
      dst += 2;
      resampler_remove_sample(resampler[0]);
    }
  }

  if (llpData)
    *llpData = src;
  return total_bytes;
}

static int SOUND_ResampleMix_S16_Stereo_Mono(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    int iBufLen,
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
  int src_samples = lpSpec->size >> 2;
  const short *src = (const short *)lpData;
  short *dst = (short *)lpBuffer;
  int channel_len = iBufLen, total_bytes = 0;

  while (total_bytes < channel_len && src_samples > 0) {
    int j, to_write = resampler_get_free_count(resampler[0]);
    if (to_write > src_samples)
      to_write = src_samples;
    for (j = 0; j < to_write; j++) {
      resampler_write_sample(resampler[0], *src++);
      resampler_write_sample(resampler[1], *src++);
    }
    src_samples -= to_write;
    while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0) {
      int sample = (((resampler_get_sample(resampler[0]) >> 8) + (resampler_get_sample(resampler[1]) >> 8)) >> 1) + *dst;
      *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
      total_bytes += sizeof(short);
      resampler_remove_sample(resampler[0]);
      resampler_remove_sample(resampler[1]);
    }
  }

  if (llpData)
    *llpData = src;
  return total_bytes;
}

static int SOUND_ResampleMix_S16_Stereo_Stereo(
    void *resampler[2],
    const void *lpData,
    const WAVESPEC *lpSpec,
    void *lpBuffer,
    int iBufLen,
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
  int src_samples = lpSpec->size >> 2;
  const short *src = (const short *)lpData;
  short *dst = (short *)lpBuffer;
  int channel_len = iBufLen >> 1, total_bytes = 0;

  while (total_bytes < channel_len && src_samples > 0) {
    int j, to_write = resampler_get_free_count(resampler[0]);
    if (to_write > src_samples)
      to_write = src_samples;
    for (j = 0; j < to_write; j++) {
      resampler_write_sample(resampler[0], *src++);
      resampler_write_sample(resampler[1], *src++);
    }
    src_samples -= to_write;
    while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0) {
      int sample;
      sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
      *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
      sample = (resampler_get_sample(resampler[1]) >> 8) + *dst;
      *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
      total_bytes += sizeof(short);
      resampler_remove_sample(resampler[0]);
      resampler_remove_sample(resampler[1]);
    }
  }

  if (llpData)
    *llpData = src;
  return total_bytes;
}

static int SOUND_Play(
    void *object,
    int iSoundNum,
    int fLoop,
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
  ResampleMixer mixer;
  WAVEDATA *cursnd;
  void *buf;
  const void *snddata;
  int len, i;

  // Check for NULL pointer.
  if (player == NULL) {
    return FALSE;
  }

  if (player->lastSFX == iSoundNum)
    return FALSE;

  player->lastSFX = iSoundNum;

  // Get the length of the sound file.
  len = PAL_MKFGetChunkSize(iSoundNum, player->mkf);
  if (len <= 0) {
    return FALSE;
  }

  // Read the sound file from the MKF archive.
  buf = UTIL_malloc(len);
  PAL_MKFReadChunk(buf, len, iSoundNum, player->mkf);

  snddata = SOUND_LoadWAVEData(buf, len, &wavespec);
  if (snddata == NULL) {
    UTIL_free(buf);
    return FALSE;
  }

  if (wavespec.channels == 1 && gConfig.iAudioChannels == 1)
    mixer = (wavespec.format) ? SOUND_ResampleMix_S16_Mono_Mono : SOUND_ResampleMix_U8_Mono_Mono;
  else if (wavespec.channels == 1 && gConfig.iAudioChannels == 2)
    mixer = (wavespec.format) ? SOUND_ResampleMix_S16_Mono_Stereo : SOUND_ResampleMix_U8_Mono_Stereo;
  else if (wavespec.channels == 2 && gConfig.iAudioChannels == 1)
    mixer = (wavespec.format) ? SOUND_ResampleMix_S16_Stereo_Mono : SOUND_ResampleMix_U8_Stereo_Mono;
  else if (wavespec.channels == 2 && gConfig.iAudioChannels == 2)
    mixer = (wavespec.format) ? SOUND_ResampleMix_S16_Stereo_Stereo : SOUND_ResampleMix_U8_Stereo_Stereo;
  else {
    UTIL_free(buf);
    return FALSE;
  }

  cursnd = &player->soundlist;
  while (cursnd->next && cursnd->base)
    cursnd = cursnd->next;
  if (cursnd->base) {
    WAVEDATA *obj = (WAVEDATA *)UTIL_malloc(sizeof(WAVEDATA));
    cursnd->next = obj;
    cursnd = cursnd->next;
  }

  for (i = 0; i < wavespec.channels; i++) {
    if (!cursnd->resampler[i])
      cursnd->resampler[i] = resampler_create();
    else
      resampler_clear(cursnd->resampler[i]);
    resampler_set_quality(cursnd->resampler[i], ((wavespec.freq % gConfig.iSampleRate) == 0 || (gConfig.iSampleRate % wavespec.freq) == 0) ? RESAMPLER_QUALITY_MIN : RESAMPLER_QUALITY_MAX);
    resampler_set_rate(cursnd->resampler[i], (double)wavespec.freq / (double)gConfig.iSampleRate);
  }

  cursnd->base = buf;
  cursnd->current = snddata;
  cursnd->end = (const uint8_t *)snddata + wavespec.size;
  cursnd->spec = wavespec;
  cursnd->ResampleMix = mixer;
  player->cursounds++;

  return TRUE;
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
  if (player) {
    WAVEDATA *cursnd = &player->soundlist;
    do {
      if (cursnd->resampler[0])
        resampler_delete(cursnd->resampler[0]);
      if (cursnd->resampler[1])
        resampler_delete(cursnd->resampler[1]);
      if (cursnd->base)
        UTIL_free((void *)cursnd->base);
    } while ((cursnd = cursnd->next) != NULL);
    cursnd = player->soundlist.next;
    while (cursnd) {
      WAVEDATA *old = cursnd;
      cursnd = cursnd->next;
      UTIL_free(old);
    }
  }
  UTIL_fclose(player->mkf);
}

static void SOUND_FillBuffer(
    void *object,
    unsigned char *stream,
    int len)
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
  if (player) {
    WAVEDATA *cursnd = &player->soundlist;
    int sounds = 0;
    do {
      if (cursnd->base) {
        cursnd->ResampleMix(cursnd->resampler, cursnd->current, &cursnd->spec, stream, len, &cursnd->current);
        cursnd->spec.size = (int)((const uint8_t *)cursnd->end - (const uint8_t *)cursnd->current);
        if (cursnd->spec.size < cursnd->spec.align) {
          UTIL_free((void *)cursnd->base);
          cursnd->base = cursnd->current = cursnd->end = NULL;
          player->cursounds--;
          player->lastSFX = 0;
        } else
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
  FILE *mkf = UTIL_fopen(RESOURCE_PATH "/sounds.mkf", "rb");
  if (mkf == NULL)
    return NULL;
  SOUNDPLAYER *player = (SOUNDPLAYER *)UTIL_malloc(sizeof(SOUNDPLAYER));
  player->Play = SOUND_Play;
  player->FillBuffer = SOUND_FillBuffer;
  player->Shutdown = SOUND_Shutdown;
  player->mkf = mkf;
  player->soundlist.resampler[0] = resampler_create();
  player->soundlist.resampler[1] = resampler_create();
  player->cursounds = 0;
  return (AUDIOPLAYER *)player;
}
