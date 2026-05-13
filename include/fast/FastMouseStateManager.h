#pragma once

#include "ship/window/MouseStateManager.h"

namespace Fast {

/**
 * @brief Fast3D-specific mouse-state manager.
 *
 * Requires a **Ship::Window** component as a direct child of the Context so
 * that ResetCursorVisibilityTimer() can query the target framerate.
 */
class FastMouseStateManager : public Ship::MouseStateManager {
  public:
    void ResetCursorVisibilityTimer() override;
    void SetCursorVisibilityTime(uint32_t seconds);
    uint32_t GetCursorVisibilityTime();

  private:
    uint32_t mCursorVisibleSeconds = 3;
};
} // namespace Fast
