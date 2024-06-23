#include "GfxDebugger.h"
#include <spdlog/fmt/fmt.h>
#include <gfxd.h>
#include <spdlog/spdlog.h>

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

#ifdef GFX_DEBUG_DISASSEMBLER

gfxd_ucode_t GfxDebugger::GetUcode(void) {
    return mSelectedUcode;
}

void GfxDebugger::SetUcode(uint32_t ucode) {
    switch(ucode) {
        case 0:
            mSelectedUcode = gfxd_f3d;
            break;
        case 1:
            mSelectedUcode = gfxd_f3dex;
            break;
        case 2:
            mSelectedUcode = gfxd_f3dex2;
            break;
        default:
            SPDLOG_ERROR("Incorrect ucode for GfxDebugger, defaulting to f3dex2");
            mSelectedUcode = gfxd_f3dex2;
            break;
    }
}
#endif

} // namespace LUS
