#pragma once

#include <stdint.h>

#ifndef LUS_VERSION_EPOCH
#define LUS_VERSION_EPOCH 0
#endif

#ifndef LUS_VERSION_MAJOR
#define LUS_VERSION_MAJOR 0
#endif

#ifndef LUS_VERSION_MINOR
#define LUS_VERSION_MINOR 0
#endif

#ifndef LUS_VERSION_PATCH
#define LUS_VERSION_PATCH 0
#endif

#define SHIP_VERSION_EPOCH LUS_VERSION_EPOCH
#define SHIP_VERSION_MAJOR LUS_VERSION_MAJOR
#define SHIP_VERSION_MINOR LUS_VERSION_MINOR
#define SHIP_VERSION_PATCH LUS_VERSION_PATCH

#define SHIP_PACK_VERSION(epoch, major, minor, patch)                                                              \
    ((((uint32_t)((epoch)&0xFF)) << 24) | (((uint32_t)((major)&0xFF)) << 16) | (((uint32_t)((minor)&0xFF)) << 8) | \
     ((uint32_t)((patch)&0xFF)))

#define SHIP_VERSION_U32 \
    SHIP_PACK_VERSION(SHIP_VERSION_EPOCH, SHIP_VERSION_MAJOR, SHIP_VERSION_MINOR, SHIP_VERSION_PATCH)

#ifdef __cplusplus
namespace Ship {
inline constexpr uint32_t Version = SHIP_VERSION_U32;
}
#endif
