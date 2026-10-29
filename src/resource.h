#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdint.h>

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

// Consolidate extracted resource files into a single resource file.
void PAL_ConsolidateExtractedResources(void);

// Load the consolidated resource file and build the resource index.
int PAL_LoadConsolidatedResources(void);

// Free the loaded resource index and close the resource file.
void PAL_FreeResourceIndex(void);

// Get the number of chunks in a resource file
uint32_t RES_MKFGetChunkCount(
    uint8_t resource_id);

// Get the size of a specific chunk in a resource file
uint32_t RES_MKFGetChunkSize(
    uint32_t chunk_index,
    uint8_t resource_id);

// Reads an animation frame into a buffer
int RES_RNGReadFrame(
    void **frame_buffer,
    uint32_t animation_index,
    uint32_t frame_index,
    uint8_t resource_id);

// Decompresses a chunk from the resource file into a buffer
uint32_t RES_MKFDecompressChunk(
    void **chunk_buffer,
    uint32_t buffer_size,
    uint32_t chunk_index,
    uint8_t resource_id);

// Reads a raw chunk from the resource file into a provided buffer.
uint32_t RES_MKFReadChunk(
    void *chunk_buffer,
    uint32_t buffer_size,
    uint32_t chunk_index,
    uint8_t resource_id);

uint32_t RES_MKFCreateChunk(
    void **chunk_buffer,
    uint32_t chunk_index,
    uint8_t resource_id);

#endif // RESOURCE_H