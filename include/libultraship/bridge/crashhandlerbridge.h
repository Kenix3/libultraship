#pragma once

#include <stddef.h>
#include "ship/Api.h"

typedef void (*CrashHandlerCallback)(char*, size_t*);

#ifdef __cplusplus
extern "C" {
#endif

API_EXPORT void CrashHandlerRegisterCallback(CrashHandlerCallback callback);

#ifdef __cplusplus
}
#endif
