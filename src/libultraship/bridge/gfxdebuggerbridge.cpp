#include "libultraship/bridge/gfxdebuggerbridge.h"
#include "ship/Context.h"
#include "fast/Fast3dWindow.h"

static std::shared_ptr<Fast::GfxDebugger> GetGfxDebugger() {
    auto window = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
    return window ? window->GetGfxDebugger() : nullptr;
}

void GfxDebuggerRequestDebugging() {
    if (auto dbg = GetGfxDebugger()) {
        dbg->RequestDebugging();
    }
}
bool GfxDebuggerIsDebugging() {
    auto dbg = GetGfxDebugger();
    return dbg ? dbg->IsDebugging() : false;
}
bool GfxDebuggerIsDebuggingRequested() {
    auto dbg = GetGfxDebugger();
    return dbg ? dbg->IsDebuggingRequested() : false;
}
void GfxDebuggerDebugDisplayList(void* cmds) {
    if (auto dbg = GetGfxDebugger()) {
        dbg->DebugDisplayList((Fast::F3DGfx*)cmds);
    }
}
