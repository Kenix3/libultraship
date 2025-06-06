#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

void GfxDebuggerRequestDebugging();
bool GfxDebuggerIsDebugging();
bool GfxDebuggerIsDebuggingRequested();
void GfxDebuggerDebugDisplayList(void* cmds);

#ifdef __cplusplus
};
#endif
