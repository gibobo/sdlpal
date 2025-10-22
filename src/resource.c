#include "resource.h"
#include "palcommon.h"
#include "rngplay.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

extern char *gFiles_name[Res_Count];

typedef struct
{
    unsigned int offset;
    unsigned int length;
} pal_file_info_t;

typedef struct
{
    unsigned int chunk_count;
    unsigned int *frame_count;
    pal_file_info_t **file_info;
} resource_export_import_t;

resource_export_import_t gResource_Export_Import[Res_Count];

static void PAL_ExtractAndDecompressMKFChunks(PALRES res)
{
    char filename_res[256] = {0};
    char filename_info[256] = {0};
    sprintf(filename_res, "%s/res_%d.bin", CACHES_PATH, (unsigned int)res);
    sprintf(filename_info, "%s/res_%d.dat", CACHES_PATH, (unsigned int)res);
    FILE *fp = UTIL_fopen(gFiles_name[res], "rb");
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
        int len = PAL_MKFDecompressChunk(&buffer, 0, index, fp);
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
    FILE *fp = UTIL_fopen(gFiles_name[res], "rb");
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
    FILE *fp = UTIL_fopen(gFiles_name[res], "rb");
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
    unsigned int offset_res = 0;
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
        fwrite(buffer_u8, sizeof(char), res_len, fpRes_out);
        UTIL_free(buffer_u8);

        // resource info concatenation
        FILE *fpInfo = UTIL_fopen(filename_info, "rb");
        unsigned int info_len = UTIL_FileLength(fpInfo);
        unsigned int *buffer_u32 = (unsigned int *)UTIL_malloc(info_len);
        UTIL_fread(buffer_u32, sizeof(char), info_len, fpInfo);
        UTIL_fclose(fpInfo);

        long curr_pos = ftell(fpInfo_out);
        offset_info = (curr_pos - Info_start_pos) / sizeof(unsigned int);
        fseek(fpInfo_out, res * sizeof(unsigned int), SEEK_SET);
        fwrite(&offset_info, sizeof(unsigned int), 1, fpInfo_out);
        fseek(fpInfo_out, curr_pos, SEEK_SET);

        unsigned int offset = 0;
        unsigned int uiChunkCount = buffer_u32[offset++];
        fwrite(&uiChunkCount, sizeof(unsigned int), 1, fpInfo_out);
        for (unsigned int i = 0; i < uiChunkCount; i++)
        {
            unsigned int frame_num = buffer_u32[offset++];
            fwrite(&frame_num, sizeof(unsigned int), 1, fpInfo_out);
            for (unsigned int j = 0; j < frame_num; j++)
            {
                unsigned int data_offset = buffer_u32[offset++] + offset_res;
                unsigned int data_length = buffer_u32[offset++];
                fwrite(&data_offset, sizeof(unsigned int), 1, fpInfo_out);
                fwrite(&data_length, sizeof(unsigned int), 1, fpInfo_out);
            }
        }
        UTIL_free(buffer_u32);

        offset_res += res_len;
    }
    UTIL_fclose(fpRes_out);
    UTIL_fclose(fpInfo_out);
}

int PAL_LoadConsolidatedResources(void)
{
    // FILE *fpRes = UTIL_fopen(CACHES_PATH "/pal.bin", "rb");
    FILE *fpInfo = UTIL_fopen(CACHES_PATH "/pal.dat", "rb");
    if (!fpInfo)
    {
        return -1; // 無法打開資源文件
    }
    unsigned int offset_info = 0;
    unsigned int info_len = UTIL_FileLength(fpInfo);
    if (info_len == 0)
    {
        UTIL_fclose(fpInfo);
        return -2; // 資源文件為空
    }
    
    unsigned int *buffer_u32 = (unsigned int *)UTIL_malloc(info_len);
    if (!buffer_u32)
    {
        UTIL_fclose(fpInfo);
        return -3; // 內存分配失敗
    }
    
    size_t read_size = UTIL_fread(buffer_u32, sizeof(char), info_len, fpInfo);
    UTIL_fclose(fpInfo);
    
    if (read_size != info_len)
    {
        UTIL_free(buffer_u32);
        return -4; // 讀取文件失敗
    }

    // 使用緩衝區數據而不是重新打開文件
    unsigned int buffer_offset = 0;
    
    // 檢查是否有足夠的數據用於偏移表
    if (info_len < Res_Count * sizeof(unsigned int))
    {
        UTIL_free(buffer_u32);
        return -5; // 文件格式錯誤：偏移表不完整
    }

    for (PALRES res = 0; res < Res_Count; res++)
    {
        // 從緩衝區讀取偏移信息
        offset_info = buffer_u32[res];
        
        // 檢查偏移是否有效
        if (offset_info >= info_len / sizeof(unsigned int))
        {
            UTIL_free(buffer_u32);
            return -6; // 文件格式錯誤：無效偏移
        }

        // 讀取chunk數量
        unsigned int uiChunkCount = buffer_u32[offset_info];
        buffer_offset = offset_info + 1;
        
        // 檢查是否有足夠的數據
        if (buffer_offset >= info_len / sizeof(unsigned int))
        {
            UTIL_free(buffer_u32);
            return -7; // 文件格式錯誤：數據不足
        }

        gResource_Export_Import[res].chunk_count = uiChunkCount;
        gResource_Export_Import[res].frame_count = (unsigned int *)UTIL_malloc(sizeof(unsigned int) * uiChunkCount);
        gResource_Export_Import[res].file_info = (pal_file_info_t **)UTIL_malloc(sizeof(pal_file_info_t *) * uiChunkCount);
        
        if (!gResource_Export_Import[res].frame_count || !gResource_Export_Import[res].file_info)
        {
            UTIL_free(buffer_u32);
            return -8; // 內存分配失敗
        }

        for (unsigned int i = 0; i < uiChunkCount; i++)
        {
            // 檢查緩衝區邊界
            if (buffer_offset >= info_len / sizeof(unsigned int))
            {
                UTIL_free(buffer_u32);
                return -9; // 文件格式錯誤：數據越界
            }
            
            // 讀取幀數量
            unsigned int frame_num = buffer_u32[buffer_offset++];
            gResource_Export_Import[res].frame_count[i] = frame_num;
            gResource_Export_Import[res].file_info[i] = (pal_file_info_t *)UTIL_malloc(sizeof(pal_file_info_t) * frame_num);
            
            if (!gResource_Export_Import[res].file_info[i])
            {
                UTIL_free(buffer_u32);
                return -10; // 內存分配失敗
            }
            
            for (unsigned int j = 0; j < frame_num; j++)
            {
                // 檢查是否有足夠的數據讀取offset和length
                if (buffer_offset + 1 >= info_len / sizeof(unsigned int))
                {
                    UTIL_free(buffer_u32);
                    return -11; // 文件格式錯誤：幀數據不完整
                }
                
                unsigned int data_offset = buffer_u32[buffer_offset++];
                unsigned int data_length = buffer_u32[buffer_offset++];
                gResource_Export_Import[res].file_info[i][j].offset = data_offset;
                gResource_Export_Import[res].file_info[i][j].length = data_length;
            }
        }
    }

    UTIL_free(buffer_u32);
    return 0; // 成功
}

void PAL_FreeResourceIndex(void)
{
    for (PALRES res = 0; res < Res_Count; res++)
    {
        for (unsigned int i = 0; i < gResource_Export_Import[res].chunk_count; i++)
        {
            UTIL_free(gResource_Export_Import[res].file_info[i]);
        }
        UTIL_free(gResource_Export_Import[res].file_info);
        UTIL_free(gResource_Export_Import[res].frame_count);
    }
}
