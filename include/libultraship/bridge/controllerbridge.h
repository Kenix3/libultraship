#pragma once

#include <stdint.h>
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif

API_EXPORT void ControllerBlockGameInput(uint16_t inputBlockId);
API_EXPORT void ControllerUnblockGameInput(uint16_t inputBlockId);

#ifdef __cplusplus
};
#endif
