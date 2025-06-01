#pragma once

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
