#pragma once

#include "stdint.h"
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif

API_EXPORT bool WindowIsRunning();
API_EXPORT uint32_t WindowGetWidth();
API_EXPORT uint32_t WindowGetHeight();
API_EXPORT float WindowGetAspectRatio();
API_EXPORT int32_t WindowGetPosX();
API_EXPORT int32_t WindowGetPosY();
API_EXPORT bool WindowIsFullscreen();

#ifdef __cplusplus
};
#endif
