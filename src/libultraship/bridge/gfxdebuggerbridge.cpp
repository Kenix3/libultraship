#include "libultraship/bridge/gfxdebuggerbridge.h"
#include "fast/debug/GfxDebugger.h"
#include <atomic>

// Dependency: requires Fast::GfxDebugger component to be present in Ship::Context.

static std::atomic<std::shared_ptr<Fast::GfxDebugger>> sGfxDebugger;

void GfxDebuggerSetComponent(std::shared_ptr<Fast::GfxDebugger> gfxDebugger) {
    sGfxDebugger.store(std::move(gfxDebugger), std::memory_order_release);
}

std::shared_ptr<Fast::GfxDebugger> GfxDebuggerGetComponent() {
    return sGfxDebugger.load(std::memory_order_acquire);
}

void GfxDebuggerRequestDebugging() {
    auto gfxDebugger = GfxDebuggerGetComponent();
    if (gfxDebugger == nullptr) {
        return;
    }

    gfxDebugger->RequestDebugging();
}
bool GfxDebuggerIsDebugging() {
    auto gfxDebugger = GfxDebuggerGetComponent();
    if (gfxDebugger == nullptr) {
        return false;
    }

    return gfxDebugger->IsDebugging();
}
bool GfxDebuggerIsDebuggingRequested() {
    auto gfxDebugger = GfxDebuggerGetComponent();
    if (gfxDebugger == nullptr) {
        return false;
    }

    return gfxDebugger->IsDebuggingRequested();
}
void GfxDebuggerDebugDisplayList(void* cmds) {
    auto gfxDebugger = GfxDebuggerGetComponent();
    if (gfxDebugger == nullptr) {
        return;
    }

    gfxDebugger->DebugDisplayList((Fast::F3DGfx*)cmds);
}
