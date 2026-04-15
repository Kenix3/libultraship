#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 8-bit RGB colour without an alpha channel.
 *
 * Each component is an unsigned byte in the range [0, 255].
 */
typedef struct {
    uint8_t r; ///< Red channel.
    uint8_t g; ///< Green channel.
    uint8_t b; ///< Blue channel.
} Color_RGB8;

/**
 * @brief 8-bit RGBA colour (red, green, blue, alpha).
 *
 * Each component is an unsigned byte in the range [0, 255].
 * An alpha value of 0 is fully transparent; 255 is fully opaque.
 */
typedef struct {
    uint8_t r; ///< Red channel.
    uint8_t g; ///< Green channel.
    uint8_t b; ///< Blue channel.
    uint8_t a; ///< Alpha channel (0 = transparent, 255 = opaque).
} Color_RGBA8;

#ifdef __cplusplus
};
#endif
