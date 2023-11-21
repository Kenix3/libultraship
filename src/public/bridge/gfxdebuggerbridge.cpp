#include "gfxdebuggerbridge.h"
#include "Context.h"
#include "debug/GfxDebugger.h"

void GfxDebuggerRequestDebugging(void) {
    LUS::Context::GetInstance()->GetGfxDebugger()->RequestDebugging();
}
bool GfxDebuggerIsDebugging(void) {
    return LUS::Context::GetInstance()->GetGfxDebugger()->IsDebugging();
}
bool GfxDebuggerIsDebuggingRequested(void) {
    return LUS::Context::GetInstance()->GetGfxDebugger()->IsDebuggingRequested();
}
void GfxDebuggerDebugDisplayList(void* cmds) {
    LUS::Context::GetInstance()->GetGfxDebugger()->DebugDisplayList((Gfx*)cmds);
}
