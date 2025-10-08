#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void GfxDebuggerRequestDebugging();
bool GfxDebuggerIsDebugging();
bool GfxDebuggerIsDebuggingRequested();
void GfxDebuggerDebugDisplayList(void* cmds);

#ifdef __cplusplus
};
#endif
