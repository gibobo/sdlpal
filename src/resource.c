#include "resource.h"
#include "palcommon.h"
#include "rngplay.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

extern char *gFiles_name[Res_Count];

static void Data_Extraction_DecompressChunk(PALRES res)
{
    char filename_res[256] = {0};
    char filename_info[256] = {0};
    sprintf(filename_res, "%s/res_%d.bin", CACHES_PATH, (unsigned int)res);
    sprintf(filename_info, "%s/res_%d.dat", CACHES_PATH, (unsigned int)res);
    FILE *fp = UTIL_fopen(gFiles_name[res], "rb");
    FILE *fpRes = UTIL_fopen(filename_res, "wb");
    FILE *fpInfo = UTIL_fopen(filename_info, "w");
    unsigned int index = 0;
    unsigned int offset = 0;
    unsigned int uiChunkCount = PAL_MKFGetChunkCount(fp);
    fwrite(&uiChunkCount, sizeof(unsigned int), 1, fpInfo);
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
        fwrite(&offset, sizeof(unsigned int), 1, fpInfo);
        fwrite(&res_len, sizeof(unsigned int), 1, fpInfo);
        offset += res_len;
        UTIL_free(buffer);
    }
    UTIL_fclose(fp);
    UTIL_fclose(fpRes);
    UTIL_fclose(fpInfo);
}

static void Data_Extraction_Chunk(PALRES res)
{
    char buffer[1024 * 480]; // 480KB buffer
    char filename_res[256] = {0};
    char filename_info[256] = {0};
    sprintf(filename_res, "%s/res_%d.bin", CACHES_PATH, (unsigned int)res);
    sprintf(filename_info, "%s/res_%d.dat", CACHES_PATH, (unsigned int)res);
    FILE *fp = UTIL_fopen(gFiles_name[res], "rb");
    FILE *fpRes = UTIL_fopen(filename_res, "wb");
    FILE *fpInfo = UTIL_fopen(filename_info, "w");
    unsigned int index = 0;
    unsigned int offset = 0;
    unsigned int uiChunkCount = PAL_MKFGetChunkCount(fp);
    fwrite(&uiChunkCount, sizeof(unsigned int), 1, fpInfo);
    for (index = 0; index < uiChunkCount; index++)
    {
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
    FILE *fpInfo = UTIL_fopen(filename_info, "w");
    unsigned int index = 0;
    unsigned int offset = 0;
    unsigned int frame_num = 0;
    unsigned int uiChunkCount = PAL_MKFGetChunkCount(fp);
    fwrite(&uiChunkCount, sizeof(unsigned int), 1, fpInfo);

    for (index = 0; index < uiChunkCount; index++)
        fwrite(&frame_num, sizeof(unsigned int), 1, fpInfo);

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

        long new_pos = ftell(fpInfo);
        fseek(fpInfo, sizeof(unsigned int) * (1 + index), SEEK_SET);
        fwrite(&frame_num, sizeof(unsigned int), 1, fpInfo);
        fseek(fpInfo, new_pos, SEEK_SET);
    }
    UTIL_fclose(fp);
    UTIL_fclose(fpRes);
    UTIL_fclose(fpInfo);
}

void PAL_ResourcesCreate(void)
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
    Data_Extraction_Chunk(Res_MUS);  //TODO
    Data_Extraction_Chunk(Res_PAT);
    Data_Extraction_Chunk(Res_RGM);
    Data_Extraction_Rng(Res_RNG);
    Data_Extraction_Chunk(Res_SOUNDS);
    Data_Extraction_Chunk(Res_SSS);
}
