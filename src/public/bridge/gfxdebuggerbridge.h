#pragma once

#ifndef GFXDEBUGGER_H
#define GFXDEBUGGER_H

#include "stdint.h"
#include "stddef.h"

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

#endif
