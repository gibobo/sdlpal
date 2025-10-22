#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdio.h>
#include <stdlib.h>

// Resource identifiers
typedef enum
{
    Res_ABC = 0, // enemy sprites in battle
    Res_BALL,    // item bitmaps
    Res_DATA,    // misc data
    Res_F,       // player sprites during battle
    Res_FBP,     // battlefield background images
    Res_FIRE,    // fire effect sprites
    Res_GOP,     // map objects
    Res_MAP,     // map data
    Res_MGO,     // sprites in scenes
    Res_MUS,     // music data
    Res_PAT,     // palette data
    Res_RGM,     // character face bitmaps
    Res_RNG,     // RNG animation data
    Res_SOUNDS,  // sound data,
    Res_SSS,     // special scene sprites
    Res_Count    // total number of resources
} PALRES;

void PAL_ConsolidateExtractedResources(void);
int PAL_LoadConsolidatedResources(void);
void PAL_FreeResourceIndex(void);

int RES_ReadAnimationFrame(
    unsigned char **frame_buffer,
    unsigned int animation_index,
    unsigned int frame_index,
    unsigned char resource_id);

int RES_MKFGetChunkSize(
    unsigned int chunk_index,
    unsigned char resource_id);

int RES_MKFDecompressChunk(
    unsigned char **chunk_buffer,
    unsigned int buffer_size,
    unsigned int chunk_index,
    unsigned char resource_id);

int RES_MKFReadChunk(
    void *output_buffer,
    unsigned int buffer_size,
    unsigned int chunk_index,
    unsigned char resource_id);

#endif // RESOURCE_H