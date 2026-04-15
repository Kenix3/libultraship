#pragma once

#include <stdint.h>

#include <ship/utils/color.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 32-bit RGBA colour stored in a union that allows component and packed-integer access.
 *
 * The byte order of the components within @c rgba depends on whether the target is big-endian
 * (IS_BIGENDIAN defined) or little-endian. Use the named struct members for portable access
 * and the @c rgba field only when a packed 32-bit representation is required (e.g. GPU uploads).
 *
 * @note Only use the @c rgba field when alignment requirements demand it.
 */
typedef union {
    struct {
#ifdef IS_BIGENDIAN
        uint8_t r; ///< Red channel (big-endian layout).
        uint8_t g; ///< Green channel (big-endian layout).
        uint8_t b; ///< Blue channel (big-endian layout).
        uint8_t a; ///< Alpha channel (big-endian layout).
#else
        uint8_t a; ///< Alpha channel (little-endian layout — lowest address byte).
        uint8_t b; ///< Blue channel (little-endian layout).
        uint8_t g; ///< Green channel (little-endian layout).
        uint8_t r; ///< Red channel (little-endian layout — highest address byte).
#endif
    };
    uint32_t rgba; ///< All four channels packed into a single 32-bit integer.
} Color_RGBA8_u32;

/**
 * @brief Single-precision floating-point RGBA colour.
 *
 * Each component is in the range [0.0f, 1.0f]:
 * - 0.0f represents the minimum intensity (black / transparent).
 * - 1.0f represents the maximum intensity (white / fully opaque).
 */
typedef struct {
    float r; ///< Red channel   [0.0, 1.0].
    float g; ///< Green channel [0.0, 1.0].
    float b; ///< Blue channel  [0.0, 1.0].
    float a; ///< Alpha channel [0.0, 1.0] (0 = transparent, 1 = opaque).
} Color_RGBAf;

/**
 * @brief 16-bit RGBA colour using 5-5-5-1 bit packing.
 *
 * - r, g, b: 5 bits each (range 0–31).
 * - a: 1 bit (0 = transparent, 1 = opaque).
 *
 * The @c rgba field gives the complete packed 16-bit value.
 */
typedef union {
    struct {
        uint16_t r : 5; ///< Red channel   (5 bits, range 0–31).
        uint16_t g : 5; ///< Green channel (5 bits, range 0–31).
        uint16_t b : 5; ///< Blue channel  (5 bits, range 0–31).
        uint16_t a : 1; ///< Alpha channel (1 bit,  0 = transparent, 1 = opaque).
    };
    uint16_t rgba; ///< All four channels packed into a single 16-bit integer.
} Color_RGBA16;

#ifdef __cplusplus
};
#endif
