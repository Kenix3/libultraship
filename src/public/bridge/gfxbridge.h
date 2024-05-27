#ifndef GFX_BRIDGE_H
#define GFX_BRIDGE_H

#include "stdint.h"

typedef enum UcodeHandlers {
    ucode_f3d,
    ucode_f3dex,
    ucode_f3dex2,
    ucode_s2dex,
    ucode_max,
} UcodeHandlers;

#ifdef __cplusplus
extern "C" {
#endif

void GfxSetNativeDimensions(uint32_t width, uint32_t height);
void GfxGetPixelDepthPrepare(float x, float y);
uint16_t GfxGetPixelDepth(float x, float y);

#ifdef __cplusplus
}
#endif

#endif
