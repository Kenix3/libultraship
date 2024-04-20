#include "GfxDebugger.h"
#include <spdlog/fmt/fmt.h>

namespace LUS {

void GfxDebugger::ResumeGame() {
    mIsDebugging = false;
    mIsDebuggingRequested = false;
    mDlist = nullptr;
}

const F3DGfx* GfxDebugger::GetDisplayList() const {
    return mDlist;
}

const std::vector<const F3DGfx*>& GfxDebugger::GetBreakPoint() const {
    return mBreakPoint;
}

void GfxDebugger::SetBreakPoint(const std::vector<const F3DGfx*>& bp) {
    mBreakPoint = bp;
}

void GfxDebugger::RequestDebugging() {
    mIsDebuggingRequested = true;
}
bool GfxDebugger::IsDebugging() const {
    return mIsDebugging;
}
bool GfxDebugger::IsDebuggingRequested() const {
    return mIsDebuggingRequested;
}

void GfxDebugger::DebugDisplayList(F3DGfx* cmds) {
    mDlist = cmds;
    mIsDebuggingRequested = false;
    mIsDebugging = true;
    mBreakPoint = { cmds };
}

bool GfxDebugger::HasBreakPoint(const std::vector<const F3DGfx*>& path) const {
    if (path.size() != mBreakPoint.size())
        return false;

    for (size_t i = 0; i < path.size(); i++) {
        if (path[i] != mBreakPoint[i]) {
            return false;
        }
    }

    return true;
}

} // namespace LUS
