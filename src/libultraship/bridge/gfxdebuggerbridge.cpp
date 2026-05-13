#include "libultraship/bridge/gfxdebuggerbridge.h"
#include "ship/Context.h"
#include "fast/debug/GfxDebugger.h"

// Dependency: requires Fast::GfxDebugger component to be present in Ship::Context.

static std::shared_ptr<Fast::GfxDebugger> sGfxDebugger;

static Fast::GfxDebugger* GetGfxDebugger() {
    if (!sGfxDebugger) {
        sGfxDebugger = Ship::Context::GetInstance()->GetChildren().GetFirst<Fast::GfxDebugger>();
    }
    return sGfxDebugger.get();
}

void GfxDebuggerRequestDebugging() {
    GetGfxDebugger()->RequestDebugging();
}
bool GfxDebuggerIsDebugging() {
    return GetGfxDebugger()->IsDebugging();
}
bool GfxDebuggerIsDebuggingRequested() {
    return GetGfxDebugger()->IsDebuggingRequested();
}
void GfxDebuggerDebugDisplayList(void* cmds) {
    GetGfxDebugger()->DebugDisplayList((Fast::F3DGfx*)cmds);
}
