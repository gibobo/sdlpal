#include "resource.h"
#include "palcommon.h"
#include "rngplay.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

static const char *g_ResourceFilePaths[Res_Count] = {
    [Res_ABC] = "abc.mkf",
    [Res_BALL] = "ball.mkf",
    [Res_DATA] = "data.mkf",
    [Res_F] = "f.mkf",
    [Res_FBP] = "fbp.mkf",
    [Res_FIRE] = "fire.mkf",
    [Res_GOP] = "gop.mkf",
    [Res_MAP] = "map.mkf",
    [Res_MGO] = "mgo.mkf",
    [Res_MUS] = "mus.mkf",
    [Res_PAT] = "pat.mkf",
    [Res_RGM] = "rgm.mkf",
    [Res_RNG] = "rng.mkf",
    [Res_SOUNDS] = "sounds.mkf",
    [Res_SSS] = "sss.mkf",
};

typedef struct
{
    unsigned int data_offset;
    unsigned int data_length;
} ResourceFileInfo;

typedef struct
{
    unsigned int chunk_count;
    unsigned int *frame_count;
    ResourceFileInfo **resource_file_info;
} ResourceIndex;

static unsigned int resource_offsets[Res_Count + 1] = {0};

/* Initialize the global resource export/import array to ensure a single
   defined object with zero-initialized contents to avoid multiple-definition
   or uninitialized-data issues across builds. */
static ResourceIndex g_CachedResourceIndex[Res_Count] = {0};

static void PAL_ExtractAndDecompressMKFChunks(PALRES res)
{
    FILE *fp = UTIL_fopen(UTIL_Filename("%s/%s", RESOURCE_PATH, g_ResourceFilePaths[res]), "rb");
    FILE *fpRes = UTIL_fopen(UTIL_Filename("%s/res_%d.bin", CACHES_PATH, (unsigned int)res), "wb");
    FILE *fpInfo = UTIL_fopen(UTIL_Filename("%s/res_%d.dat", CACHES_PATH, (unsigned int)res), "wb");
    unsigned int index = 0;
    unsigned int offset = 0;
    unsigned int frame_num = 1;
    unsigned int uiChunkCount = PAL_MKFGetChunkCount(fp);
    fwrite(&uiChunkCount, sizeof(uiChunkCount), 1, fpInfo);
    for (index = 0; index < uiChunkCount; index++)
    {
        unsigned char *buffer = NULL;
        unsigned int len = PAL_MKFDecompressChunk(&buffer, 0, index, fp);
        if (len < 0)
        {
            break;
        }

        unsigned int res_len = (unsigned int)len;
        if (buffer != NULL && res_len > 0)
            fwrite(buffer, sizeof(char), res_len, fpRes);
        fwrite(&frame_num, sizeof(unsigned int), 1, fpInfo);
        fwrite(&offset, sizeof(unsigned int), 1, fpInfo);
        fwrite(&res_len, sizeof(unsigned int), 1, fpInfo);
        offset += res_len;
        UTIL_free(buffer);
    }
    UTIL_fclose(fp);
    UTIL_fclose(fpRes);
    UTIL_fclose(fpInfo);
}

static void PAL_ExtractRawMKFChunks(PALRES res)
{
    FILE *fp = UTIL_fopen(UTIL_Filename("%s/%s", RESOURCE_PATH, g_ResourceFilePaths[res]), "rb");
    FILE *fpRes = UTIL_fopen(UTIL_Filename("%s/res_%d.bin", CACHES_PATH, (unsigned int)res), "wb");
    FILE *fpInfo = UTIL_fopen(UTIL_Filename("%s/res_%d.dat", CACHES_PATH, (unsigned int)res), "wb");
    unsigned int index = 0;
    unsigned int offset = 0;
    unsigned int frame_num = 1;
    unsigned int uiChunkCount = PAL_MKFGetChunkCount(fp);
    fwrite(&uiChunkCount, sizeof(uiChunkCount), 1, fpInfo);
    for (index = 0; index < uiChunkCount; index++)
    {
        char buffer[1024 * 480]; // 480KB buffer
        memset(buffer, 0, sizeof(buffer));
        int len = PAL_MKFReadChunk(buffer, sizeof(buffer), index, fp);
        if (len < 0)
        {
            break;
        }

        unsigned int res_len = (unsigned int)len;
        fwrite(buffer, sizeof(char), res_len, fpRes);
        fwrite(&frame_num, sizeof(unsigned int), 1, fpInfo);
        fwrite(&offset, sizeof(unsigned int), 1, fpInfo);
        fwrite(&res_len, sizeof(unsigned int), 1, fpInfo);
        offset += res_len;
    }
    UTIL_fclose(fp);
    UTIL_fclose(fpRes);
    UTIL_fclose(fpInfo);
}

static void PAL_ExtractRNGAnimationFrames(PALRES res)
{
    FILE *fp = UTIL_fopen(UTIL_Filename("%s/%s", RESOURCE_PATH, g_ResourceFilePaths[res]), "rb");
    FILE *fpRes = UTIL_fopen(UTIL_Filename("%s/res_%d.bin", CACHES_PATH, (unsigned int)res), "wb");
    FILE *fpInfo = UTIL_fopen(UTIL_Filename("%s/res_%d.dat", CACHES_PATH, (unsigned int)res), "wb");
    unsigned int index = 0;
    unsigned int offset = 0;
    unsigned int frame_num = 0;
    unsigned int uiChunkCount = PAL_MKFGetChunkCount(fp);
    fwrite(&uiChunkCount, sizeof(uiChunkCount), 1, fpInfo);
    for (index = 0; index < uiChunkCount; index++)
    {
        frame_num = 0;
        long fn_pos = ftell(fpInfo);
        fwrite(&frame_num, sizeof(unsigned int), 1, fpInfo);
        while (1)
        {
            unsigned char *buf = NULL;
            int buf_size = PAL_RNGReadFrame(&buf, index, frame_num++, fp);
            if (buf_size <= 0)
            {
                UTIL_free(buf);
                break; // Failed to get the frame, don't go further
            }
            unsigned int rng_size = *(unsigned int *)buf;
            unsigned char *rng = (unsigned char *)UTIL_malloc(rng_size);
            unsigned int RNGBlit_len = YJ2_Decompress(buf, rng, rng_size);
            fwrite(rng, sizeof(char), RNGBlit_len, fpRes);
            fwrite(&offset, sizeof(unsigned int), 1, fpInfo);
            fwrite(&RNGBlit_len, sizeof(unsigned int), 1, fpInfo);
            offset += rng_size;
            UTIL_free(rng);
            UTIL_free(buf);
        }
        frame_num--;
        long curr_pos = ftell(fpInfo);
        fseek(fpInfo, fn_pos, SEEK_SET);
        fwrite(&frame_num, sizeof(unsigned int), 1, fpInfo);
        fseek(fpInfo, curr_pos, SEEK_SET);
    }
    UTIL_fclose(fp);
    UTIL_fclose(fpRes);
    UTIL_fclose(fpInfo);
}

void PAL_ConsolidateExtractedResources(void)
{
    PAL_ExtractAndDecompressMKFChunks(Res_ABC);
    PAL_ExtractRawMKFChunks(Res_BALL);
    PAL_ExtractRawMKFChunks(Res_DATA);
    PAL_ExtractAndDecompressMKFChunks(Res_F);
    PAL_ExtractAndDecompressMKFChunks(Res_FBP);
    PAL_ExtractAndDecompressMKFChunks(Res_FIRE);
    PAL_ExtractRawMKFChunks(Res_GOP);
    PAL_ExtractAndDecompressMKFChunks(Res_MAP);
    PAL_ExtractAndDecompressMKFChunks(Res_MGO);
    PAL_ExtractRawMKFChunks(Res_MUS); //TODO
    PAL_ExtractRawMKFChunks(Res_PAT);
    PAL_ExtractRawMKFChunks(Res_RGM);
    PAL_ExtractRNGAnimationFrames(Res_RNG);
    PAL_ExtractRawMKFChunks(Res_SOUNDS);
    PAL_ExtractRawMKFChunks(Res_SSS);

    FILE *fpRes_out = UTIL_fopen(UTIL_Filename("%s/pal.bin", CACHES_PATH), "wb");
    FILE *fpInfo_out = UTIL_fopen(UTIL_Filename("%s/pal.dat", CACHES_PATH), "wb");
    unsigned int offset_info = 0;
    long Info_start_pos = ftell(fpInfo_out);

    for (PALRES res = 0; res < Res_Count; res++)
        fwrite(&offset_info, sizeof(unsigned int), 1, fpInfo_out);

    for (PALRES res = 0; res < Res_Count; res++)
    {
        // resource file concatenation
        FILE *fpRes = UTIL_fopen(UTIL_Filename("%s/res_%d.bin", CACHES_PATH, (unsigned int)res), "rb");
        unsigned int res_len = UTIL_FileLength(fpRes);
        unsigned char *buffer_u8 = (unsigned char *)UTIL_malloc(res_len);
        UTIL_fread(buffer_u8, sizeof(char), res_len, fpRes);
        UTIL_fclose(fpRes);
        UTIL_fwrite(buffer_u8, sizeof(char), res_len, fpRes_out);
        UTIL_free(buffer_u8);

        // resource info concatenation
        FILE *fpInfo = UTIL_fopen(UTIL_Filename("%s/res_%d.dat", CACHES_PATH, (unsigned int)res), "rb");
        unsigned int info_len = UTIL_FileLength(fpInfo);
        unsigned int *buffer_u32 = (unsigned int *)UTIL_malloc(info_len);
        UTIL_fread(buffer_u32, sizeof(char), info_len, fpInfo);
        UTIL_fclose(fpInfo);

        long curr_pos = ftell(fpInfo_out);
        offset_info = (curr_pos - Info_start_pos) / sizeof(unsigned int);
        UTIL_fseek(fpInfo_out, res * sizeof(unsigned int), SEEK_SET);
        UTIL_fwrite(&offset_info, sizeof(unsigned int), 1, fpInfo_out);
        UTIL_fseek(fpInfo_out, curr_pos, SEEK_SET);
        UTIL_fwrite(buffer_u32, sizeof(char), info_len, fpInfo_out);
        UTIL_free(buffer_u32);
    }
    UTIL_fclose(fpRes_out);
    UTIL_fclose(fpInfo_out);
}

int PAL_LoadConsolidatedResources(void)
{
    FILE *fpInfo = UTIL_fopen(UTIL_Filename("%s/pal.dat", CACHES_PATH), "rb");
    if (!fpInfo)
    {
        return -1; // Failed to open resource file
    }
    unsigned int offset_info = 0;
    unsigned int info_len = UTIL_FileLength(fpInfo);
    if (info_len == 0)
    {
        UTIL_fclose(fpInfo);
        return -2; // Resource file is empty
    }

    unsigned int *buffer_u32 = (unsigned int *)UTIL_malloc(info_len);
    if (!buffer_u32)
    {
        UTIL_fclose(fpInfo);
        return -3; // Memory allocation failed
    }

    size_t read_size = UTIL_fread(buffer_u32, sizeof(char), info_len, fpInfo);
    UTIL_fclose(fpInfo);

    if (read_size != info_len)
    {
        UTIL_free(buffer_u32);
        return -4; // Failed to read file
    }

    // Use buffer data instead of reopening file
    unsigned int buffer_offset = 0;

    // Check if there's enough data for offset table
    if (info_len < Res_Count * sizeof(unsigned int))
    {
        UTIL_free(buffer_u32);
        return -5; // File format error: incomplete offset table
    }
    // unsigned int max_data_len[Res_Count][2] = {0};
    for (PALRES res = 0; res < Res_Count; res++)
    {
        unsigned int res_size = 0;
        // Read offset information from buffer
        offset_info = buffer_u32[res];

        // Check if offset is valid
        if (offset_info >= info_len / sizeof(unsigned int))
        {
            UTIL_free(buffer_u32);
            return -6; // File format error: invalid offset
        }

        // Read chunk count
        unsigned int uiChunkCount = buffer_u32[offset_info];
        buffer_offset = offset_info + 1;

        // Check if there's enough data
        if (buffer_offset >= info_len / sizeof(unsigned int))
        {
            UTIL_free(buffer_u32);
            return -7; // File format error: insufficient data
        }

        g_CachedResourceIndex[res].chunk_count = uiChunkCount;
        g_CachedResourceIndex[res].frame_count = (unsigned int *)UTIL_malloc(sizeof(unsigned int) * uiChunkCount);
        g_CachedResourceIndex[res].resource_file_info = (ResourceFileInfo **)UTIL_malloc(sizeof(ResourceFileInfo *) * uiChunkCount);

        if (!g_CachedResourceIndex[res].frame_count || !g_CachedResourceIndex[res].resource_file_info)
        {
            UTIL_free(buffer_u32);
            return -8; // Memory allocation failed
        }

        for (unsigned int i = 0; i < uiChunkCount; i++)
        {
            // Check buffer bounds
            if (buffer_offset >= info_len / sizeof(unsigned int))
            {
                UTIL_free(buffer_u32);
                return -9; // File format error: data out of bounds
            }

            // Read frame count
            unsigned int frame_num = buffer_u32[buffer_offset++];
            g_CachedResourceIndex[res].frame_count[i] = frame_num;
            g_CachedResourceIndex[res].resource_file_info[i] = (ResourceFileInfo *)UTIL_malloc(sizeof(ResourceFileInfo) * frame_num);

            if (!g_CachedResourceIndex[res].resource_file_info[i])
            {
                UTIL_free(buffer_u32);
                return -10; // Memory allocation failed
            }

            for (unsigned int j = 0; j < frame_num; j++)
            {
                // Check if there's enough data to read offset and length
                if (buffer_offset + 1 >= info_len / sizeof(unsigned int))
                {
                    UTIL_free(buffer_u32);
                    return -11; // File format error: incomplete frame data
                }

                unsigned int data_offset = buffer_u32[buffer_offset++];
                unsigned int data_length = buffer_u32[buffer_offset++];
                g_CachedResourceIndex[res].resource_file_info[i][j].data_offset = data_offset;
                g_CachedResourceIndex[res].resource_file_info[i][j].data_length = data_length;
                res_size += data_length;
                // if (data_length > 0 && (max_data_len[res][0] == 0 || max_data_len[res][0] > data_length))
                //     max_data_len[res][0] = data_length;
                // if (max_data_len[res][1] < data_length)
                //     max_data_len[res][1] = data_length;
            }
        }
        // resource_offsets[res + 1] = resource_offsets[res] + res_size;
    }

    UTIL_free(buffer_u32);
    return 0; // Success
}

void PAL_FreeResourceIndex(void)
{
    for (PALRES res = 0; res < Res_Count; res++)
    {
        for (unsigned int i = 0; i < g_CachedResourceIndex[res].chunk_count; i++)
        {
            UTIL_free(g_CachedResourceIndex[res].resource_file_info[i]);
        }
        UTIL_free(g_CachedResourceIndex[res].resource_file_info);
        UTIL_free(g_CachedResourceIndex[res].frame_count);
        g_CachedResourceIndex[res].chunk_count = 0;
        g_CachedResourceIndex[res].frame_count = NULL;
        g_CachedResourceIndex[res].resource_file_info = NULL;
    }
}

int RES_MKFGetChunkSize(
    unsigned int chunk_index,
    unsigned char resource_id)
{
    if (chunk_index >= g_CachedResourceIndex[resource_id].chunk_count)
        return 0;

    return g_CachedResourceIndex[resource_id].resource_file_info[chunk_index][0].data_length;
}

int RES_ReadAnimationFrame(
    void **frame_buffer,
    unsigned int animation_index,
    unsigned int frame_index,
    unsigned char resource_id)
{
    static void *fpRes = NULL;
    static unsigned int p_data_length = 0;
    unsigned int data_length = g_CachedResourceIndex[resource_id].resource_file_info[animation_index][frame_index].data_length;
    char filename[256] = {0};

    if (animation_index >= g_CachedResourceIndex[resource_id].chunk_count ||
        frame_index >= g_CachedResourceIndex[resource_id].frame_count[animation_index] ||
        data_length == 0)
    {
        UTIL_free(*frame_buffer);
        UTIL_fclose(fpRes);
        *frame_buffer = NULL;
        fpRes = NULL;
        p_data_length = 0;
        return 0;
    }

    if (p_data_length < data_length)
    {
        p_data_length = data_length;
        UTIL_free(*frame_buffer);
    }

    *frame_buffer = UTIL_malloc(data_length);

    if (fpRes == NULL)
    {
        sprintf(filename, "%s/res_%d.bin", CACHES_PATH, (unsigned int)resource_id);
        fpRes = UTIL_fopen(filename, "rb");
        UTIL_fseek(fpRes,
                   resource_offsets[resource_id] + g_CachedResourceIndex[resource_id].resource_file_info[animation_index][frame_index].data_offset,
                   SEEK_SET);
    }
    UTIL_fread(*frame_buffer, 1, data_length, fpRes);

    return data_length;
}

unsigned int RES_MKFDecompressChunk(
    void **chunk_buffer,
    unsigned int buffer_size,
    unsigned int chunk_index,
    unsigned char resource_id)
{
    unsigned int data_length = g_CachedResourceIndex[resource_id].resource_file_info[chunk_index][0].data_length;

    if (g_CachedResourceIndex[resource_id].chunk_count <= chunk_index || data_length == 0)
        return 0;

    if (buffer_size == 0 || buffer_size < data_length)
    {
        UTIL_free(*chunk_buffer);
        *chunk_buffer = NULL;
    }

    if (*chunk_buffer == NULL)
    {
        *chunk_buffer = UTIL_malloc(data_length);
    }

    return RES_MKFReadChunk(
        *chunk_buffer,
        data_length,
        chunk_index,
        resource_id);
}

unsigned int RES_MKFReadChunk(
    void *chunk_buffer,
    unsigned int buffer_size,
    unsigned int chunk_index,
    unsigned char resource_id)
{
    unsigned int data_length = g_CachedResourceIndex[resource_id].resource_file_info[chunk_index][0].data_length;

    if (g_CachedResourceIndex[resource_id].chunk_count <= chunk_index || data_length == 0)
        return 0;

    if (buffer_size == 0 || buffer_size < data_length || chunk_buffer == NULL)
        return 0;

    char filename[256] = {0};
    sprintf(filename, "%s/res_%d.bin", CACHES_PATH, (unsigned int)resource_id);
    void *fpRes = UTIL_fopen(filename, "rb");
    UTIL_fseek(fpRes,
               resource_offsets[resource_id] + g_CachedResourceIndex[resource_id].resource_file_info[chunk_index][0].data_offset,
               SEEK_SET);
    UTIL_fread(chunk_buffer, 1, data_length, fpRes);
    UTIL_fclose(fpRes);

    return data_length;
}
