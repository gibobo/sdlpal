#include "resource.h"
#include "palcommon.h"
#include "rngplay.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

static const char *g_ResourceFilePaths[Res_Count] = {
    [Res_ABC] = RESOURCE_PATH "/abc.mkf",
    [Res_BALL] = RESOURCE_PATH "/ball.mkf",
    [Res_DATA] = RESOURCE_PATH "/data.mkf",
    [Res_F] = RESOURCE_PATH "/f.mkf",
    [Res_FBP] = RESOURCE_PATH "/fbp.mkf",
    [Res_FIRE] = RESOURCE_PATH "/fire.mkf",
    [Res_GOP] = RESOURCE_PATH "/gop.mkf",
    [Res_MAP] = RESOURCE_PATH "/map.mkf",
    [Res_MGO] = RESOURCE_PATH "/mgo.mkf",
    [Res_MUS] = RESOURCE_PATH "/mus.mkf",
    [Res_PAT] = RESOURCE_PATH "/pat.mkf",
    [Res_RGM] = RESOURCE_PATH "/rgm.mkf",
    [Res_RNG] = RESOURCE_PATH "/rng.mkf",
    [Res_SOUNDS] = RESOURCE_PATH "/sounds.mkf",
    [Res_SSS] = RESOURCE_PATH "/sss.mkf",
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

static char filename[256] = {0};
static unsigned char g_resource_id = 0xff;
static void *g_ConsolidatedResourceFile = NULL;
static unsigned int resource_offsets[Res_Count + 1] = {0};

/* Initialize the global resource export/import array to ensure a single
   defined object with zero-initialized contents to avoid multiple-definition
   or uninitialized-data issues across builds. */
static ResourceIndex g_CachedResourceIndex[Res_Count] = {0};

static void PAL_ExtractAndDecompressMKFChunks(PALRES res)
{
    char filename_res[256] = {0};
    char filename_info[256] = {0};
    sprintf(filename_res, "%s/res_%d.bin", CACHES_PATH, (unsigned int)res);
    sprintf(filename_info, "%s/res_%d.dat", CACHES_PATH, (unsigned int)res);
    FILE *fp = UTIL_fopen(g_ResourceFilePaths[res], "rb");
    FILE *fpRes = UTIL_fopen(filename_res, "wb");
    FILE *fpInfo = UTIL_fopen(filename_info, "wb");
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
    char filename_res[256] = {0};
    char filename_info[256] = {0};
    sprintf(filename_res, "%s/res_%d.bin", CACHES_PATH, (unsigned int)res);
    sprintf(filename_info, "%s/res_%d.dat", CACHES_PATH, (unsigned int)res);
    FILE *fp = UTIL_fopen(g_ResourceFilePaths[res], "rb");
    FILE *fpRes = UTIL_fopen(filename_res, "wb");
    FILE *fpInfo = UTIL_fopen(filename_info, "wb");
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
    char filename_res[256] = {0};
    char filename_info[256] = {0};
    sprintf(filename_res, "%s/res_%d.bin", CACHES_PATH, (unsigned int)res);
    sprintf(filename_info, "%s/res_%d.dat", CACHES_PATH, (unsigned int)res);
    FILE *fp = UTIL_fopen(g_ResourceFilePaths[res], "rb");
    FILE *fpRes = UTIL_fopen(filename_res, "wb");
    FILE *fpInfo = UTIL_fopen(filename_info, "wb");
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

    char filename_res[256] = {0};
    char filename_info[256] = {0};
    sprintf(filename_res, "%s/pal.bin", CACHES_PATH);
    sprintf(filename_info, "%s/pal.dat", CACHES_PATH);
    FILE *fpRes_out = UTIL_fopen(filename_res, "wb");
    FILE *fpInfo_out = UTIL_fopen(filename_info, "wb");
    unsigned int offset_info = 0;
    long Info_start_pos = ftell(fpInfo_out);

    for (PALRES res = 0; res < Res_Count; res++)
        fwrite(&offset_info, sizeof(unsigned int), 1, fpInfo_out);

    for (PALRES res = 0; res < Res_Count; res++)
    {
        sprintf(filename_res, "%s/res_%d.bin", CACHES_PATH, (unsigned int)res);
        sprintf(filename_info, "%s/res_%d.dat", CACHES_PATH, (unsigned int)res);

        // resource file concatenation
        FILE *fpRes = UTIL_fopen(filename_res, "rb");
        unsigned int res_len = UTIL_FileLength(fpRes);
        unsigned char *buffer_u8 = (unsigned char *)UTIL_malloc(res_len);
        UTIL_fread(buffer_u8, sizeof(char), res_len, fpRes);
        UTIL_fclose(fpRes);
        UTIL_fwrite(buffer_u8, sizeof(char), res_len, fpRes_out);
        UTIL_free(buffer_u8);

        // resource info concatenation
        FILE *fpInfo = UTIL_fopen(filename_info, "rb");
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
    FILE *fpInfo = UTIL_fopen(CACHES_PATH "/pal.dat", "rb");
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
    UTIL_fclose(g_ConsolidatedResourceFile);
    g_ConsolidatedResourceFile = NULL;
    g_resource_id = 0xff;

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
    if (animation_index >= g_CachedResourceIndex[resource_id].chunk_count)
        return -1;
    if (frame_index >= g_CachedResourceIndex[resource_id].frame_count[animation_index])
        return -2;

    static unsigned int p_frame_index = 0xFFFF;
    unsigned int data_length = g_CachedResourceIndex[resource_id].resource_file_info[animation_index][frame_index].data_length;
    if (data_length > 0)
    {
        UTIL_free(*frame_buffer);
        *frame_buffer = UTIL_malloc(data_length);
        if (g_resource_id != resource_id)
        {
            p_frame_index = 0xFFFF;
            g_resource_id = resource_id;
            UTIL_fclose(g_ConsolidatedResourceFile);
            sprintf(filename, "%s/res_%d.bin", CACHES_PATH, (unsigned int)resource_id);
            g_ConsolidatedResourceFile = UTIL_fopen(filename, "rb");
        }
        if (p_frame_index + 1 != frame_index)
            UTIL_fseek(g_ConsolidatedResourceFile,
                       resource_offsets[resource_id] + g_CachedResourceIndex[resource_id].resource_file_info[animation_index][frame_index].data_offset,
                       SEEK_SET);
        UTIL_fread(*frame_buffer, 1, data_length, g_ConsolidatedResourceFile);
        p_frame_index = frame_index;
    }
    else
    {
        UTIL_free(*frame_buffer);
        *frame_buffer = NULL;
    }

    return data_length;
}

unsigned int RES_MKFDecompressChunk(
    void **chunk_buffer,
    unsigned int buffer_size,
    unsigned int chunk_index,
    unsigned char resource_id)
{
    if (g_CachedResourceIndex[resource_id].chunk_count <= chunk_index)
        return 0;

    unsigned int data_length = g_CachedResourceIndex[resource_id].resource_file_info[chunk_index][0].data_length;

    if (data_length)
    {
        if (buffer_size == 0 || buffer_size != data_length || *chunk_buffer == NULL)
        {
            UTIL_free(*chunk_buffer);
            buffer_size = data_length;
            *chunk_buffer = UTIL_malloc(buffer_size);
        }
        if (g_resource_id != resource_id)
        {
            g_resource_id = resource_id;
            UTIL_fclose(g_ConsolidatedResourceFile);
            sprintf(filename, "%s/res_%d.bin", CACHES_PATH, (unsigned int)resource_id);
            g_ConsolidatedResourceFile = UTIL_fopen(filename, "rb");
        }
        UTIL_fseek(g_ConsolidatedResourceFile,
                   resource_offsets[resource_id] + g_CachedResourceIndex[resource_id].resource_file_info[chunk_index][0].data_offset,
                   SEEK_SET);
        UTIL_fread(*chunk_buffer, 1, buffer_size, g_ConsolidatedResourceFile);
    }

    return data_length;
}

unsigned int RES_MKFReadChunk(
    void *output_buffer,
    unsigned int buffer_size,
    unsigned int chunk_index,
    unsigned char resource_id)
{
    if (g_CachedResourceIndex[resource_id].chunk_count <= chunk_index)
        return 0;

    unsigned int data_length = g_CachedResourceIndex[resource_id].resource_file_info[chunk_index][0].data_length;

    if (data_length)
    {
        if (buffer_size == 0 || data_length > buffer_size || output_buffer == NULL)
            return 0;

        if (g_resource_id != resource_id)
        {
            g_resource_id = resource_id;
            UTIL_fclose(g_ConsolidatedResourceFile);
            sprintf(filename, "%s/res_%d.bin", CACHES_PATH, (unsigned int)resource_id);
            g_ConsolidatedResourceFile = UTIL_fopen(filename, "rb");
        }
        UTIL_fseek(g_ConsolidatedResourceFile,
                   resource_offsets[resource_id] + g_CachedResourceIndex[resource_id].resource_file_info[chunk_index][0].data_offset,
                   SEEK_SET);
        UTIL_fread(output_buffer, 1, data_length, g_ConsolidatedResourceFile);
    }
    return data_length;
}
