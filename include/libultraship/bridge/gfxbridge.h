#pragma once

#include "stdint.h"
#include "fast/ucodehandlers.h"

#ifdef __cplusplus
extern "C" {
#endif

void GfxSetNativeDimensions(uint32_t width, uint32_t height);
void GfxGetPixelDepthPrepare(float x, float y);
uint16_t GfxGetPixelDepth(float x, float y);

#ifdef __cplusplus
}
#endif
