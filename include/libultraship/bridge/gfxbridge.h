#pragma once

#include "stdint.h"
#include "fast/ucodehandlers.h"
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif

API_EXPORT void GfxSetNativeDimensions(uint32_t width, uint32_t height);
API_EXPORT void GfxGetPixelDepthPrepare(float x, float y);
API_EXPORT uint16_t GfxGetPixelDepth(float x, float y);

#ifdef __cplusplus
}
#endif
