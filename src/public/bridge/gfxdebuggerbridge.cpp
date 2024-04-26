#include "gfxdebuggerbridge.h"
#include "Context.h"
#include "debug/GfxDebugger.h"

void GfxDebuggerRequestDebugging(void) {
    ShipDK::Context::GetInstance()->GetGfxDebugger()->RequestDebugging();
}
bool GfxDebuggerIsDebugging(void) {
    return ShipDK::Context::GetInstance()->GetGfxDebugger()->IsDebugging();
}
bool GfxDebuggerIsDebuggingRequested(void) {
    return ShipDK::Context::GetInstance()->GetGfxDebugger()->IsDebuggingRequested();
}
void GfxDebuggerDebugDisplayList(void* cmds) {
    ShipDK::Context::GetInstance()->GetGfxDebugger()->DebugDisplayList((Gfx*)cmds);
}
