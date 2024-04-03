#pragma once

#include <string>
#include <vector>

extern "C" {
#include "libultraship/libultra/gbi.h"
}

namespace LUS {

class GfxDebugger {
  public:
    void RequestDebugging();
    bool IsDebugging() const;
    bool IsDebuggingRequested() const;

    void DebugDisplayList(Gfx* cmds);

    void ResumeGame();

    const Gfx* GetDisplayList() const;

    const std::vector<const Gfx*>& GetBreakPoint() const;

    bool HasBreakPoint(const std::vector<const Gfx*>& path) const;

    void SetBreakPoint(const std::vector<const Gfx*>& bp);

  private:
    bool mIsDebugging = false;
    bool mIsDebuggingRequested = false;
    Gfx* mDlist = nullptr;
    std::vector<const Gfx*> mBreakPoint = {};
};

} // namespace LUS
