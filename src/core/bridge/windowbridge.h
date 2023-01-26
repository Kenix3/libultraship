#pragma once

#ifndef WINDOWBRIDGE_H
#define WINDOWBRIDGE_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t GetWindowWidth();
uint32_t GetWindowHeight();
float GetWindowAspectRatio();
void GetPixelDepthPrepare(float x, float y);
uint16_t GetPixelDepth(float x, float y);
uint32_t DoesOtrFileExist();

#ifdef __cplusplus
};
#endif

#endif