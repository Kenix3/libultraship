#pragma once

#ifndef WINDOWBRIDGE_H
#define WINDOWBRIDGE_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

bool WindowIsRunning();
uint32_t WindowGetWidth();
uint32_t WindowGetHeight();
float WindowGetAspectRatio();
int32_t WindowGetPosX();
int32_t WindowGetPosY();
bool WindowIsFullscreen();

#ifdef __cplusplus
};
#endif

#endif