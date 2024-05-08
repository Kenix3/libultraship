#pragma once

#ifndef WINDOWBRIDGE_H
#define WINDOWBRIDGE_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

bool WindowIsRunning(void);
uint32_t WindowGetWidth(void);
uint32_t WindowGetHeight(void);
float WindowGetAspectRatio(void);
int32_t WindowGetPosX(void);
int32_t WindowGetPosY(void);
bool WindowIsFullscreen(void);

#ifdef __cplusplus
};
#endif

#endif