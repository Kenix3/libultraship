#pragma once

#include <vector>
#include "ship/Component.h"

namespace Fast {
union F3DGfx;

/**
 * @brief Graphics display-list debugger for the Fast3D renderer.
 *
 * GfxDebugger allows pausing execution at a specific display-list command,
 * inspecting the current display list, and stepping through rendering.
 * It is used by the GfxDebuggerWindow GUI panel.
 *
 * **Required Context children:** None — GfxDebugger has no dependencies on other
 * components. However, it must be present as a **direct child of the Context**
 * because the Fast3D interpreter looks it up via
 * `Context::GetChildren().GetFirst<GfxDebugger>()` on every rendered frame.
 *
 * Obtain the instance from `Context::GetChildren().GetFirst<Fast::GfxDebugger>()`.
 */
class GfxDebugger : public Ship::Component {
  public:
    /** @brief Constructs the GfxDebugger component. */
    GfxDebugger();

    /** @brief Requests that the renderer pause after the next display list. */
    void RequestDebugging();

    /** @brief Returns true if the debugger is currently paused on a display list. */
    bool IsDebugging() const;

    /** @brief Returns true if a debugging pause has been requested but not yet honoured. */
    bool IsDebuggingRequested() const;

    /**
     * @brief Captures a display list for debugging inspection.
     * @param cmds Pointer to the head of the display list to debug.
     */
    void DebugDisplayList(F3DGfx* cmds);

    /** @brief Resumes game execution after a debugging pause. */
    void ResumeGame();

    /**
     * @brief Returns the currently captured display list.
     * @return Pointer to the display list head, or nullptr if none is captured.
     */
    const F3DGfx* GetDisplayList() const;

    /**
     * @brief Returns the current breakpoint path.
     * @return Vector of display-list command pointers forming the breakpoint path.
     */
    const std::vector<const F3DGfx*>& GetBreakPoint() const;

    /**
     * @brief Checks whether a breakpoint is set at the given command path.
     * @param path Display-list command path to check.
     * @return true if a breakpoint exists at the path.
     */
    bool HasBreakPoint(const std::vector<const F3DGfx*>& path) const;

    /**
     * @brief Sets a breakpoint at the given display-list command path.
     * @param bp Display-list command path where execution should pause.
     */
    void SetBreakPoint(const std::vector<const F3DGfx*>& bp);

  private:
    bool mIsDebugging = false;
    bool mIsDebuggingRequested = false;
    F3DGfx* mDlist = nullptr;
    std::vector<const F3DGfx*> mBreakPoint = {};
};

} // namespace Fast
