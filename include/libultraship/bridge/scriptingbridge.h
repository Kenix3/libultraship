#pragma once

#include "stdint.h"
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif

API_EXPORT void* ScriptGetFunction(const char* module, const char* function);

#ifdef __cplusplus
}
#endif
