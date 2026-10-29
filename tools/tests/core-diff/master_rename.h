#ifndef CORE_DIFF_MASTER_RENAME_H
#define CORE_DIFF_MASTER_RENAME_H

// Force-included into every upstream translation unit so its external symbols get
// a master_ prefix and can be linked next to the development implementations.

#define PAL_RLEBlitToSurface           master_PAL_RLEBlitToSurface
#define PAL_RLEBlitToSurfaceWithShadow master_PAL_RLEBlitToSurfaceWithShadow
#define PAL_RLEBlitWithColorShift      master_PAL_RLEBlitWithColorShift
#define PAL_RLEBlitMonoColor           master_PAL_RLEBlitMonoColor
#define PAL_FBPBlitToSurface           master_PAL_FBPBlitToSurface
#define PAL_RLEGetWidth                master_PAL_RLEGetWidth
#define PAL_RLEGetHeight               master_PAL_RLEGetHeight
#define PAL_SpriteGetNumFrames         master_PAL_SpriteGetNumFrames
#define PAL_SpriteGetFrame             master_PAL_SpriteGetFrame
#define PAL_MKFGetChunkCount           master_PAL_MKFGetChunkCount
#define PAL_MKFGetChunkSize            master_PAL_MKFGetChunkSize
#define PAL_MKFReadChunk               master_PAL_MKFReadChunk
#define PAL_MKFGetDecompressedSize     master_PAL_MKFGetDecompressedSize
#define PAL_MKFDecompressChunk         master_PAL_MKFDecompressChunk
#define YJ1_Decompress                 master_YJ1_Decompress
#define YJ2_Decompress                 master_YJ2_Decompress
#define Decompress                     master_Decompress
#define gConfig                        master_gConfig
#define RandomLong                     master_RandomLong
#define RandomFloat                    master_RandomFloat

#endif
