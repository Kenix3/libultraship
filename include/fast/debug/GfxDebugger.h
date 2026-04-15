#pragma once

#include <vector>
#include "ship/Component.h"

namespace Fast {
union F3DGfx;

class GfxDebugger : public Ship::Component {
  public:
    GfxDebugger();
    void RequestDebugging();
    bool IsDebugging() const;
    bool IsDebuggingRequested() const;

    void DebugDisplayList(F3DGfx* cmds);

    void ResumeGame();

    const F3DGfx* GetDisplayList() const;

    const std::vector<const F3DGfx*>& GetBreakPoint() const;

    bool HasBreakPoint(const std::vector<const F3DGfx*>& path) const;

    void SetBreakPoint(const std::vector<const F3DGfx*>& bp);

  private:
    bool mIsDebugging = false;
    bool mIsDebuggingRequested = false;
    F3DGfx* mDlist = nullptr;
    std::vector<const F3DGfx*> mBreakPoint = {};
};

} // namespace Fast
