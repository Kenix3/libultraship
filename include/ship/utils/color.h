#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t r, g, b;
} Color_RGB8;

typedef struct {
    uint8_t r, g, b, a;
} Color_RGBA8;

#ifdef __cplusplus
};
#endif
