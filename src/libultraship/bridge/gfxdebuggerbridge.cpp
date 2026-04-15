#include "libultraship/bridge/gfxdebuggerbridge.h"
#include "ship/Context.h"
#include "fast/debug/GfxDebugger.h"

void GfxDebuggerRequestDebugging() {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Fast::GfxDebugger>()->RequestDebugging();
}
bool GfxDebuggerIsDebugging() {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Fast::GfxDebugger>()->IsDebugging();
}
bool GfxDebuggerIsDebuggingRequested() {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Fast::GfxDebugger>()->IsDebuggingRequested();
}
void GfxDebuggerDebugDisplayList(void* cmds) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Fast::GfxDebugger>()->DebugDisplayList((Fast::F3DGfx*)cmds);
}
