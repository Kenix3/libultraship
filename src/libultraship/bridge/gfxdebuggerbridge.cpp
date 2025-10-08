#include "libultraship/bridge/gfxdebuggerbridge.h"
#include "ship/Context.h"
#include "fast/debug/GfxDebugger.h"

void GfxDebuggerRequestDebugging() {
    Ship::Context::GetInstance()->GetGfxDebugger()->RequestDebugging();
}
bool GfxDebuggerIsDebugging() {
    return Ship::Context::GetInstance()->GetGfxDebugger()->IsDebugging();
}
bool GfxDebuggerIsDebuggingRequested() {
    return Ship::Context::GetInstance()->GetGfxDebugger()->IsDebuggingRequested();
}
void GfxDebuggerDebugDisplayList(void* cmds) {
    Ship::Context::GetInstance()->GetGfxDebugger()->DebugDisplayList((Fast::F3DGfx*)cmds);
}
