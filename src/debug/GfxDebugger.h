#pragma once

#include <string>
#include <vector>
#ifdef GFX_DEBUG_DISASSEMBLER
#include <gfxd.h>
#endif

union F3DGfx;

namespace LUS {

class GfxDebugger {
  public:
    void RequestDebugging();
    bool IsDebugging() const;
    bool IsDebuggingRequested() const;

    void DebugDisplayList(F3DGfx* cmds);

    void ResumeGame();

    const F3DGfx* GetDisplayList() const;

    const std::vector<const F3DGfx*>& GetBreakPoint() const;

    bool HasBreakPoint(const std::vector<const F3DGfx*>& path) const;

    void SetBreakPoint(const std::vector<const F3DGfx*>& bp);
#ifdef GFX_DEBUG_DISASSEMBLER
    gfxd_ucode_t mSelectedUcode;
    void SetUcode(uint32_t);
    gfxd_ucode_t GetUcode(void);
#endif

  private:
    bool mIsDebugging = false;
    bool mIsDebuggingRequested = false;
    F3DGfx* mDlist = nullptr;
    std::vector<const F3DGfx*> mBreakPoint = {};
};

} // namespace LUS
