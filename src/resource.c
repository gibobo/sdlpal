#include "resource.h"
#include "palcommon.h"
#include "rngplay.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

// #define USING_RESOURCE_CACHE

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

#ifdef USING_RESOURCE_CACHE
typedef struct
{
    uint32_t data_offset;
    uint32_t data_length;
} ResourceFileInfo;

typedef struct
{
    uint32_t chunk_count;
    uint32_t *frame_count;
    ResourceFileInfo **resource_file_info;
} ResourceIndex;

static uint32_t resource_offsets[Res_Count + 1] = {0};

/* Initialize the global resource export/import array to ensure a single
   defined object with zero-initialized contents to avoid multiple-definition
   or uninitialized-data issues across builds. */
static ResourceIndex g_CachedResourceIndex[Res_Count] = {0};
#else
static FILE *g_ResourceFileHandles[Res_Count] = {0};
#endif

#ifdef USING_RESOURCE_CACHE
static void PAL_ExtractAndDecompressMKFChunks(PALRES res)
{
    FILE *fp = UTIL_fopen(UTIL_Filename("%s/%s", RESOURCE_PATH, g_ResourceFilePaths[res]), "rb");
    FILE *fpRes = UTIL_fopen(UTIL_Filename("%s/res_%d.bin", CACHES_PATH, (uint32_t)res), "wb");
    FILE *fpInfo = UTIL_fopen(UTIL_Filename("%s/res_%d.dat", CACHES_PATH, (uint32_t)res), "wb");
    uint32_t index = 0;
    uint32_t offset = 0;
    uint32_t frame_num = 1;
    uint32_t uiChunkCount = PAL_MKFGetChunkCount(fp);
    fwrite(&uiChunkCount, sizeof(uiChunkCount), 1, fpInfo);
    for (index = 0; index < uiChunkCount; index++)
    {
        uint8_t *buffer = NULL;
        uint32_t len = PAL_MKFDecompressChunk(&buffer, 0, index, fp);
        if (len < 0)
        {
            break;
        }

        uint32_t res_len = (uint32_t)len;
        if (buffer != NULL && res_len > 0)
            fwrite(buffer, sizeof(char), res_len, fpRes);
        fwrite(&frame_num, sizeof(uint32_t), 1, fpInfo);
        fwrite(&offset, sizeof(uint32_t), 1, fpInfo);
        fwrite(&res_len, sizeof(uint32_t), 1, fpInfo);
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
    FILE *fpRes = UTIL_fopen(UTIL_Filename("%s/res_%d.bin", CACHES_PATH, (uint32_t)res), "wb");
    FILE *fpInfo = UTIL_fopen(UTIL_Filename("%s/res_%d.dat", CACHES_PATH, (uint32_t)res), "wb");
    uint32_t index = 0;
    uint32_t offset = 0;
    uint32_t frame_num = 1;
    uint32_t uiChunkCount = PAL_MKFGetChunkCount(fp);
    fwrite(&uiChunkCount, sizeof(uiChunkCount), 1, fpInfo);
    for (index = 0; index < uiChunkCount; index++)
    {
        char buffer[1024 * 480]; // 480KB buffer
        memset(buffer, 0, sizeof(buffer));
        int32_t len = PAL_MKFReadChunk(buffer, sizeof(buffer), index, fp);
        if (len < 0)
        {
            break;
        }

        uint32_t res_len = (uint32_t)len;
        fwrite(buffer, sizeof(char), res_len, fpRes);
        fwrite(&frame_num, sizeof(uint32_t), 1, fpInfo);
        fwrite(&offset, sizeof(uint32_t), 1, fpInfo);
        fwrite(&res_len, sizeof(uint32_t), 1, fpInfo);
        offset += res_len;
    }
    UTIL_fclose(fp);
    UTIL_fclose(fpRes);
    UTIL_fclose(fpInfo);
}

static void PAL_ExtractRNGAnimationFrames(PALRES res)
{
    FILE *fp = UTIL_fopen(UTIL_Filename("%s/%s", RESOURCE_PATH, g_ResourceFilePaths[res]), "rb");
    FILE *fpRes = UTIL_fopen(UTIL_Filename("%s/res_%d.bin", CACHES_PATH, (uint32_t)res), "wb");
    FILE *fpInfo = UTIL_fopen(UTIL_Filename("%s/res_%d.dat", CACHES_PATH, (uint32_t)res), "wb");
    uint32_t index = 0;
    uint32_t offset = 0;
    uint32_t frame_num = 0;
    uint32_t uiChunkCount = PAL_MKFGetChunkCount(fp);
    fwrite(&uiChunkCount, sizeof(uiChunkCount), 1, fpInfo);
    for (index = 0; index < uiChunkCount; index++)
    {
        frame_num = 0;
        int64_t fn_pos = ftell(fpInfo);
        fwrite(&frame_num, sizeof(uint32_t), 1, fpInfo);
        while (1)
        {
            uint8_t *buf = NULL;
            int32_t buf_size = PAL_RNGReadFrame(&buf, index, frame_num++, fp);
            if (buf_size <= 0)
            {
                UTIL_free(buf);
                break; // Failed to get the frame, don't go further
            }
            uint32_t rng_size = *(uint32_t *)buf;
            uint8_t *rng = (uint8_t *)UTIL_malloc(rng_size);
            YJ2_Decompress(buf, rng);
            fwrite(rng, sizeof(char), rng_size, fpRes);
            fwrite(&offset, sizeof(uint32_t), 1, fpInfo);
            fwrite(&rng_size, sizeof(uint32_t), 1, fpInfo);
            offset += rng_size;
            UTIL_free(rng);
            UTIL_free(buf);
        }
        frame_num--;
        int64_t curr_pos = ftell(fpInfo);
        fseek(fpInfo, fn_pos, SEEK_SET);
        fwrite(&frame_num, sizeof(uint32_t), 1, fpInfo);
        fseek(fpInfo, curr_pos, SEEK_SET);
    }
    UTIL_fclose(fp);
    UTIL_fclose(fpRes);
    UTIL_fclose(fpInfo);
}
#endif

void PAL_ConsolidateExtractedResources(void)
{
#ifdef USING_RESOURCE_CACHE
    PAL_ExtractAndDecompressMKFChunks(Res_ABC);
    PAL_ExtractRawMKFChunks(Res_BALL);
    PAL_ExtractRawMKFChunks(Res_DATA);
    PAL_ExtractAndDecompressMKFChunks(Res_F);
    PAL_ExtractAndDecompressMKFChunks(Res_FBP);
    PAL_ExtractAndDecompressMKFChunks(Res_FIRE);
    PAL_ExtractRawMKFChunks(Res_GOP);
    PAL_ExtractAndDecompressMKFChunks(Res_MAP);
    PAL_ExtractAndDecompressMKFChunks(Res_MGO);
    PAL_ExtractRawMKFChunks(Res_MUS);
    PAL_ExtractRawMKFChunks(Res_PAT);
    PAL_ExtractRawMKFChunks(Res_RGM);
    PAL_ExtractRNGAnimationFrames(Res_RNG);
    PAL_ExtractRawMKFChunks(Res_SOUNDS);
    PAL_ExtractRawMKFChunks(Res_SSS);

    FILE *fpRes_out = UTIL_fopen(UTIL_Filename("%s/resource.bin", CACHES_PATH), "wb");
    FILE *fpInfo_out = UTIL_fopen(UTIL_Filename("%s/resource.dat", CACHES_PATH), "wb");
    uint32_t offset_info = 0;
    int64_t Info_start_pos = ftell(fpInfo_out);

    for (PALRES res = 0; res < Res_Count; res++)
        fwrite(&offset_info, sizeof(uint32_t), 1, fpInfo_out);

    for (PALRES res = 0; res < Res_Count; res++)
    {
        // resource file concatenation
        FILE *fpRes = UTIL_fopen(UTIL_Filename("%s/res_%d.bin", CACHES_PATH, (uint32_t)res), "rb");
        uint32_t res_len = UTIL_FileLength(fpRes);
        uint8_t *buffer_u8 = (uint8_t *)UTIL_malloc(res_len);
        UTIL_fread(buffer_u8, sizeof(char), res_len, fpRes);
        UTIL_fclose(fpRes);
        UTIL_fwrite(buffer_u8, sizeof(char), res_len, fpRes_out);
        UTIL_free(buffer_u8);

        // resource info concatenation
        FILE *fpInfo = UTIL_fopen(UTIL_Filename("%s/res_%d.dat", CACHES_PATH, (uint32_t)res), "rb");
        uint32_t info_len = UTIL_FileLength(fpInfo);
        uint32_t *buffer_u32 = (uint32_t *)UTIL_malloc(info_len);
        UTIL_fread(buffer_u32, sizeof(char), info_len, fpInfo);
        UTIL_fclose(fpInfo);

        int64_t curr_pos = ftell(fpInfo_out);
        offset_info = (curr_pos - Info_start_pos) / sizeof(uint32_t);
        UTIL_fseek(fpInfo_out, res * sizeof(uint32_t), SEEK_SET);
        UTIL_fwrite(&offset_info, sizeof(uint32_t), 1, fpInfo_out);
        UTIL_fseek(fpInfo_out, curr_pos, SEEK_SET);
        UTIL_fwrite(buffer_u32, sizeof(char), info_len, fpInfo_out);
        UTIL_free(buffer_u32);
    }
    UTIL_fclose(fpRes_out);
    UTIL_fclose(fpInfo_out);
#endif
}

int PAL_LoadConsolidatedResources(void)
{
#ifdef USING_RESOURCE_CACHE
    FILE *fpInfo = UTIL_fopen(UTIL_Filename("%s/resource.dat", CACHES_PATH), "rb");
    if (!fpInfo)
    {
        return -1; // Failed to open resource file
    }
    uint32_t offset_info = 0;
    uint32_t info_len = UTIL_FileLength(fpInfo);
    if (info_len == 0)
    {
        UTIL_fclose(fpInfo);
        return -2; // Resource file is empty
    }

    uint32_t *buffer_u32 = (uint32_t *)UTIL_malloc(info_len);
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
    uint32_t buffer_offset = 0;

    // Check if there's enough data for offset table
    if (info_len < Res_Count * sizeof(uint32_t))
    {
        UTIL_free(buffer_u32);
        return -5; // File format error: incomplete offset table
    }
    // uint32_t max_data_len[Res_Count][2] = {0};
    for (PALRES res = 0; res < Res_Count; res++)
    {
        uint32_t res_size = 0;
        // Read offset information from buffer
        offset_info = buffer_u32[res];

        // Check if offset is valid
        if (offset_info >= info_len / sizeof(uint32_t))
        {
            UTIL_free(buffer_u32);
            return -6; // File format error: invalid offset
        }

        // Read chunk count
        uint32_t uiChunkCount = buffer_u32[offset_info];
        buffer_offset = offset_info + 1;

        // Check if there's enough data
        if (buffer_offset >= info_len / sizeof(uint32_t))
        {
            UTIL_free(buffer_u32);
            return -7; // File format error: insufficient data
        }

        g_CachedResourceIndex[res].chunk_count = uiChunkCount;
        g_CachedResourceIndex[res].frame_count = (uint32_t *)UTIL_malloc(sizeof(uint32_t) * uiChunkCount);
        g_CachedResourceIndex[res].resource_file_info = (ResourceFileInfo **)UTIL_malloc(sizeof(ResourceFileInfo *) * uiChunkCount);

        if (!g_CachedResourceIndex[res].frame_count || !g_CachedResourceIndex[res].resource_file_info)
        {
            UTIL_free(buffer_u32);
            return -8; // Memory allocation failed
        }

        for (uint32_t i = 0; i < uiChunkCount; i++)
        {
            // Check buffer bounds
            if (buffer_offset >= info_len / sizeof(uint32_t))
            {
                UTIL_free(buffer_u32);
                return -9; // File format error: data out of bounds
            }

            // Read frame count
            uint32_t frame_num = buffer_u32[buffer_offset++];
            g_CachedResourceIndex[res].frame_count[i] = frame_num;
            g_CachedResourceIndex[res].resource_file_info[i] = (ResourceFileInfo *)UTIL_malloc(sizeof(ResourceFileInfo) * frame_num);

            if (!g_CachedResourceIndex[res].resource_file_info[i])
            {
                UTIL_free(buffer_u32);
                return -10; // Memory allocation failed
            }

            for (uint32_t j = 0; j < frame_num; j++)
            {
                // Check if there's enough data to read offset and length
                if (buffer_offset + 1 >= info_len / sizeof(uint32_t))
                {
                    UTIL_free(buffer_u32);
                    return -11; // File format error: incomplete frame data
                }

                uint32_t data_offset = buffer_u32[buffer_offset++];
                uint32_t data_length = buffer_u32[buffer_offset++];
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
#else
    for (PALRES res = 0; res < Res_Count; res++)
    {
        g_ResourceFileHandles[res] = UTIL_fopen(UTIL_Filename("%s/%s", RESOURCE_PATH, g_ResourceFilePaths[res]), "rb");
    }
#endif
    return 0; // Success
}

void PAL_FreeResourceIndex(void)
{
#ifdef USING_RESOURCE_CACHE
    for (PALRES res = 0; res < Res_Count; res++)
    {
        for (uint32_t i = 0; i < g_CachedResourceIndex[res].chunk_count; i++)
        {
            UTIL_free(g_CachedResourceIndex[res].resource_file_info[i]);
        }
        UTIL_free(g_CachedResourceIndex[res].resource_file_info);
        UTIL_free(g_CachedResourceIndex[res].frame_count);
        g_CachedResourceIndex[res].chunk_count = 0;
        g_CachedResourceIndex[res].frame_count = NULL;
        g_CachedResourceIndex[res].resource_file_info = NULL;
    }
#else
    for (PALRES res = 0; res < Res_Count; res++)
    {
        if (g_ResourceFileHandles[res] != NULL)
        {
            UTIL_fclose(g_ResourceFileHandles[res]);
            g_ResourceFileHandles[res] = NULL;
        }
    }
#endif
}

uint32_t RES_MKFGetChunkCount(
    uint8_t resource_id)
{
    if (resource_id >= Res_Count)
        return 0;
#ifdef USING_RESOURCE_CACHE
    return g_CachedResourceIndex[resource_id].chunk_count;
#else
    return PAL_MKFGetChunkCount(g_ResourceFileHandles[resource_id]);
#endif
}

uint32_t RES_MKFGetChunkSize(
    uint32_t chunk_index,
    uint8_t resource_id)
{
#ifdef USING_RESOURCE_CACHE
    if (chunk_index >= g_CachedResourceIndex[resource_id].chunk_count)
        return 0;

    return g_CachedResourceIndex[resource_id].resource_file_info[chunk_index][0].data_length;
#else
    if (resource_id >= Res_Count || g_ResourceFileHandles[resource_id] == NULL)
        return 0;
    return PAL_MKFGetChunkSize(chunk_index, g_ResourceFileHandles[resource_id]);
#endif
}

int RES_RNGReadFrame(
    void **frame_buffer,
    uint32_t animation_index,
    uint32_t frame_index,
    uint8_t resource_id)
{
#ifdef USING_RESOURCE_CACHE
    static void *fpRes = NULL;
    static uint32_t p_data_length = 0;
    uint32_t data_length = g_CachedResourceIndex[resource_id].resource_file_info[animation_index][frame_index].data_length;
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
        *frame_buffer = NULL;
    }

    if (*frame_buffer == NULL)
        *frame_buffer = UTIL_malloc(data_length);

    if (fpRes == NULL)
    {
        sprintf(filename, "%s/res_%d.bin", CACHES_PATH, (uint32_t)resource_id);
        fpRes = UTIL_fopen(filename, "rb");
        UTIL_fseek(fpRes,
                   resource_offsets[resource_id] + g_CachedResourceIndex[resource_id].resource_file_info[animation_index][frame_index].data_offset,
                   SEEK_SET);
    }
    UTIL_fread(*frame_buffer, 1, data_length, fpRes);

    return data_length;
#else
    if (resource_id >= Res_Count || g_ResourceFileHandles[resource_id] == NULL)
        return 0;

    uint8_t *buf = NULL;
    int32_t buf_size = PAL_RNGReadFrame(&buf, animation_index, frame_index, g_ResourceFileHandles[resource_id]);
    if (buf_size < 0 || buf == NULL)
        return 0;

    UTIL_free(*frame_buffer);
    uint32_t rng_size = *(uint32_t *)buf;
    *frame_buffer = (uint8_t *)UTIL_malloc(rng_size);
    if (!YJ2_Decompress(buf, *frame_buffer))
    {
        UTIL_free(*frame_buffer);
        *frame_buffer = NULL;
        rng_size = 0;
    }
    UTIL_free(buf);
    return rng_size;
#endif
}

uint32_t RES_MKFDecompressChunk(
    void **chunk_buffer,
    uint32_t buffer_size,
    uint32_t chunk_index,
    uint8_t resource_id)
{
#ifdef USING_RESOURCE_CACHE
    // Validate chunk index
    if (resource_id >= Res_Count || g_CachedResourceIndex[resource_id].chunk_count <= chunk_index)
        return 0;

    uint32_t data_length = g_CachedResourceIndex[resource_id].resource_file_info[chunk_index][0].data_length;

    // Check for zero-length chunk
    if (data_length == 0)
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
#else
    if (resource_id >= Res_Count || g_ResourceFileHandles[resource_id] == NULL)
        return 0;
    return PAL_MKFDecompressChunk(
        (uint8_t **)chunk_buffer,
        buffer_size,
        chunk_index,
        g_ResourceFileHandles[resource_id]);
#endif
}

uint32_t RES_MKFReadChunk(
    void *chunk_buffer,
    uint32_t buffer_size,
    uint32_t chunk_index,
    uint8_t resource_id)
{
#ifdef USING_RESOURCE_CACHE
    // Validate chunk index
    if (resource_id >= Res_Count || g_CachedResourceIndex[resource_id].chunk_count <= chunk_index)
        return 0;

    uint32_t data_length = g_CachedResourceIndex[resource_id].resource_file_info[chunk_index][0].data_length;

    if (data_length == 0 || buffer_size == 0 || buffer_size < data_length || chunk_buffer == NULL)
        return 0;

    char filename[256] = {0};
    sprintf(filename, "%s/res_%d.bin", CACHES_PATH, (uint32_t)resource_id);
    void *fpRes = UTIL_fopen(filename, "rb");
    UTIL_fseek(fpRes,
               resource_offsets[resource_id] + g_CachedResourceIndex[resource_id].resource_file_info[chunk_index][0].data_offset,
               SEEK_SET);
    UTIL_fread(chunk_buffer, 1, data_length, fpRes);
    UTIL_fclose(fpRes);

    return data_length;
#else
    if (resource_id >= Res_Count || g_ResourceFileHandles[resource_id] == NULL)
        return 0;
    return PAL_MKFReadChunk(
        chunk_buffer,
        buffer_size,
        chunk_index,
        g_ResourceFileHandles[resource_id]);
#endif
}

uint32_t RES_MKFCreateChunk(
    void **chunk_buffer,
    uint32_t chunk_index,
    uint8_t resource_id)
{
    UTIL_free(*chunk_buffer);
    uint32_t buffer_size = RES_MKFGetChunkSize(chunk_index, resource_id);
    if (buffer_size == 0)
        return 0;
    *chunk_buffer = UTIL_malloc(buffer_size);
    return buffer_size;
}
