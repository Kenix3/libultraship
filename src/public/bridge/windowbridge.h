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

float ScreenGetAspectRatio(void);
float LeftEdgeAlign(float v);
int16_t LeftEdgeRectAlign(float v);
float RightEdgeAlign(float v);
int16_t RightEdgeRectAlign(float v);
uint32_t WindowGetGameRenderWidth();
uint32_t WindowGetGameRenderHeight();

#ifdef __cplusplus
};
#endif

#endif
