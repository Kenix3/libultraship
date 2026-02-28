#pragma once

#include <memory>

namespace Ship {
class MouseStateManager {
  public:
    MouseStateManager();
    virtual ~MouseStateManager();

    virtual void StartFrame();

    virtual bool ShouldAutoCaptureMouse();
    virtual void SetAutoCaptureMouse(bool capture);
    virtual bool ShouldForceCursorVisibility();
    virtual void SetForceCursorVisibility(bool visible);

    virtual void ToggleMouseCaptureOverride();
    virtual void UpdateMouseCapture();

    virtual void ResetCursorVisibilityTimer();
    void SetCursorVisibilityTimeTicks(uint32_t ticks);
    uint32_t GetCursorVisibilityTimeTicks();

  protected:
    void CursorVisibilityTimeoutTick();

  private:
    bool mAutoCaptureMouse = false;
    bool mForceCursorVisibility = false;

    uint32_t mCursorVisibleTicks = 180;
    uint32_t mCursorVisibleTicksCounter = mCursorVisibleTicks;
};
} // namespace Ship
