#include "resource.h"
#include "palcommon.h"
#include "rngplay.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

extern char *gFiles_name[Res_Count];

static void Set_FrameNum(FILE *fpInfo, unsigned int index, unsigned int frame_num)
{
    long new_pos = ftell(fpInfo);
    fseek(fpInfo, sizeof(unsigned int) * (1 + index), SEEK_SET);
    fwrite(&frame_num, sizeof(unsigned int), 1, fpInfo);
    fseek(fpInfo, new_pos, SEEK_SET);
}

static void Data_Extraction_DecompressChunk(PALRES res)
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
        fwrite(&frame_num, sizeof(frame_num), 1, fpInfo);
    for (index = 0; index < uiChunkCount; index++)
    {
        frame_num = 0;
        unsigned char *buffer = NULL;
        int len = PAL_MKFDecompressChunk(&buffer, 0, index, fp);
        if (len < 0)
        {
            break;
        }

        unsigned int res_len = (unsigned int)len;
        if (buffer != NULL && res_len > 0)
            fwrite(buffer, sizeof(char), res_len, fpRes);
        fwrite(&offset, sizeof(unsigned int), 1, fpInfo);
        fwrite(&res_len, sizeof(unsigned int), 1, fpInfo);
        offset += res_len;
        UTIL_free(buffer);
        Set_FrameNum(fpInfo, index, res_len ? 1 : 0);
    }
    UTIL_fclose(fp);
    UTIL_fclose(fpRes);
    UTIL_fclose(fpInfo);
}

static void Data_Extraction_Chunk(PALRES res)
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
        fwrite(&frame_num, sizeof(frame_num), 1, fpInfo);
    for (index = 0; index < uiChunkCount; index++)
    {
        frame_num = 0;
        char buffer[1024 * 480]; // 480KB buffer
        memset(buffer, 0, sizeof(buffer));
        int len = PAL_MKFReadChunk(buffer, sizeof(buffer), index, fp);
        if (len < 0)
        {
            break;
        }

        unsigned int res_len = (unsigned int)len;
        fwrite(buffer, sizeof(char), res_len, fpRes);
        fwrite(&offset, sizeof(unsigned int), 1, fpInfo);
        fwrite(&res_len, sizeof(unsigned int), 1, fpInfo);
        offset += res_len;
        Set_FrameNum(fpInfo, index, res_len ? 1 : 0);
    }
    UTIL_fclose(fp);
    UTIL_fclose(fpRes);
    UTIL_fclose(fpInfo);
}

static void Data_Extraction_Rng(PALRES res)
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
        fwrite(&frame_num, sizeof(frame_num), 1, fpInfo);
    for (index = 0; index < uiChunkCount; index++)
    {
        frame_num = 0;
        while (1)
        {
            unsigned char *buf = NULL;
            unsigned char *rng = NULL;
            int buf_size = PAL_RNGReadFrame(&buf, index, frame_num++, fp);
            if (buf_size <= 0)
            {
                UTIL_free(buf);
                UTIL_free(rng);
                break; // Failed to get the frame, don't go further
            }
            unsigned int rng_size = *(unsigned int *)buf;
            rng = (unsigned char *)UTIL_malloc(rng_size);
            unsigned int RNGBlit_len = YJ2_Decompress(buf, rng, rng_size);
            fwrite(rng, sizeof(char), RNGBlit_len, fpRes);
            fwrite(&offset, sizeof(unsigned int), 1, fpInfo);
            fwrite(&rng_size, sizeof(unsigned int), 1, fpInfo);
            offset += rng_size;
            UTIL_free(rng);
            UTIL_free(buf);
        }
        Set_FrameNum(fpInfo, index, frame_num);
    }
    UTIL_fclose(fp);
    UTIL_fclose(fpRes);
    UTIL_fclose(fpInfo);
}

void PAL_ResourcesExport(void)
{
    Data_Extraction_DecompressChunk(Res_ABC);
    Data_Extraction_Chunk(Res_BALL);
    Data_Extraction_Chunk(Res_DATA);
    Data_Extraction_DecompressChunk(Res_F);
    Data_Extraction_DecompressChunk(Res_FBP);
    Data_Extraction_DecompressChunk(Res_FIRE);
    Data_Extraction_Chunk(Res_GOP);
    Data_Extraction_DecompressChunk(Res_MAP);
    Data_Extraction_DecompressChunk(Res_MGO);
    Data_Extraction_Chunk(Res_MUS); //TODO
    Data_Extraction_Chunk(Res_PAT);
    Data_Extraction_Chunk(Res_RGM);
    Data_Extraction_Rng(Res_RNG);
    Data_Extraction_Chunk(Res_SOUNDS);
    Data_Extraction_Chunk(Res_SSS);

    char filename_res[256] = {0};
    char filename_info[256] = {0};
    sprintf(filename_res, "%s/pal.bin", CACHES_PATH);
    sprintf(filename_info, "%s/pal.dat", CACHES_PATH);
    FILE *fpRes_out = UTIL_fopen(filename_res, "wb");
    FILE *fpInfo_out = UTIL_fopen(filename_info, "w");
    for (int res = 0; res < Res_Count; res++)
    {
        sprintf(filename_res, "%s/res_%d.bin", CACHES_PATH, (unsigned int)res);
        sprintf(filename_info, "%s/res_%d.dat", CACHES_PATH, (unsigned int)res);
        FILE *fpRes = UTIL_fopen(filename_res, "rb");
        FILE *fpInfo = UTIL_fopen(filename_info, "rb");
        // unsigned int res_len = UTIL_FileLength(fpRes);
        // fwrite(&res_len, sizeof(unsigned int), 1, fpInfo_out);
        // unsigned char *buffer = (unsigned char *)UTIL_malloc(res_len);
        // UTIL_fread(buffer, sizeof(char), res_len, fpRes);
        // fwrite(buffer, sizeof(char), res_len, fpRes_out);
        // UTIL_free(buffer);

        // unsigned int info_len = UTIL_FileLength(fpInfo);
        // fwrite(&info_len, sizeof(unsigned int), 1, fpInfo_out);
        // buffer = (unsigned char *)UTIL_malloc(info_len);
        // UTIL_fread(buffer, sizeof(char), info_len, fpInfo);
        // fwrite(buffer, sizeof(char), info_len, fpInfo_out);
        // UTIL_free(buffer);

        UTIL_fclose(fpRes);
        UTIL_fclose(fpInfo);
    }
    UTIL_fclose(fpRes_out);
    UTIL_fclose(fpInfo_out);
}

void PAL_ResourcesImport(void)
{
    // To be implemented: Import resources from extracted files
}
