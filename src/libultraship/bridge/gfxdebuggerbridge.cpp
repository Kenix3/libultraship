#include "libultraship/bridge/gfxdebuggerbridge.h"
#include "ship/Context.h"
#include "fast/debug/GfxDebugger.h"

void GfxDebuggerRequestDebugging() {
    Ship::Context::GetInstance()->GetChild<Fast::GfxDebugger>()->RequestDebugging();
}
bool GfxDebuggerIsDebugging() {
    return Ship::Context::GetInstance()->GetChild<Fast::GfxDebugger>()->IsDebugging();
}
bool GfxDebuggerIsDebuggingRequested() {
    return Ship::Context::GetInstance()->GetChild<Fast::GfxDebugger>()->IsDebuggingRequested();
}
void GfxDebuggerDebugDisplayList(void* cmds) {
    Ship::Context::GetInstance()->GetChild<Fast::GfxDebugger>()->DebugDisplayList((Fast::F3DGfx*)cmds);
}
