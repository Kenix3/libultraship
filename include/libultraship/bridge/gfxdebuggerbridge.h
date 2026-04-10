#pragma once

#include <stddef.h>
#include <stdint.h>
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif

API_EXPORT void GfxDebuggerRequestDebugging();
API_EXPORT bool GfxDebuggerIsDebugging();
API_EXPORT bool GfxDebuggerIsDebuggingRequested();
API_EXPORT void GfxDebuggerDebugDisplayList(void* cmds);

#ifdef __cplusplus
};
#endif
