#pragma once

#include "ship/window/MouseStateManager.h"

namespace Fast {
class FastMouseStateManager : public Ship::MouseStateManager {
  public:
    void ResetCursorVisibilityTimer() override;
    void SetCursorVisibilityTime(uint32_t seconds);
    uint32_t GetCursorVisibilityTime();

  private:
    uint32_t mCursorVisibleSeconds = 3;
};
} // namespace Fast
