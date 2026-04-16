#pragma once

#include "stdint.h"
#include "fast/ucodehandlers.h"
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sets the native (un-scaled) rendering resolution used by the graphics backend.
 *
 * @param width  Native framebuffer width in pixels.
 * @param height Native framebuffer height in pixels.
 */
API_EXPORT void GfxSetNativeDimensions(uint32_t width, uint32_t height);

/**
 * @brief Prepares the graphics backend to sample the pixel depth at screen coordinate (@p x, @p y).
 *
 * Call this before GfxGetPixelDepth() to ensure the depth value is ready.
 *
 * @param x Screen X coordinate in pixels.
 * @param y Screen Y coordinate in pixels.
 */
API_EXPORT void GfxGetPixelDepthPrepare(float x, float y);

/**
 * @brief Returns the pixel depth at screen coordinate (@p x, @p y).
 *
 * Must be called after GfxGetPixelDepthPrepare() for the same coordinates.
 *
 * @param x Screen X coordinate in pixels.
 * @param y Screen Y coordinate in pixels.
 * @return Depth value in the range [0, 65535] (16-bit fixed-point).
 */
API_EXPORT uint16_t GfxGetPixelDepth(float x, float y);

#ifdef __cplusplus
}
#endif
