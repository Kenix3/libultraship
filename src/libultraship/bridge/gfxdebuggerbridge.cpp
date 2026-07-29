#include "libultraship/bridge/gfxdebuggerbridge.h"
#include "fast/debug/GfxDebugger.h"

// Dependency: requires Fast::GfxDebugger component to be present in Ship::Context.

static std::shared_ptr<Fast::GfxDebugger> sGfxDebugger;

void GfxDebuggerSetComponent(std::shared_ptr<Fast::GfxDebugger> gfxDebugger) {
    sGfxDebugger = std::move(gfxDebugger);
}

std::shared_ptr<Fast::GfxDebugger> GfxDebuggerGetComponent() {
    return sGfxDebugger;
}

static Fast::GfxDebugger* GetGfxDebugger() {
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
