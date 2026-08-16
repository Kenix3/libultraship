#include <memory>

#include "fast/FastMouseStateManager.h"
#include "fast/Fast3dWindow.h"

namespace Fast {
void FastMouseStateManager::ResetCursorVisibilityTimer() {
    auto fast3dWindow = std::dynamic_pointer_cast<Fast3dWindow>(mWindow);
    SetCursorVisibilityTimeTicks(mCursorVisibleSeconds * fast3dWindow->GetTargetFps());
    MouseStateManager::ResetCursorVisibilityTimer();
}

void FastMouseStateManager::SetCursorVisibilityTime(uint32_t seconds) {
    mCursorVisibleSeconds = seconds;
}

uint32_t FastMouseStateManager::GetCursorVisibilityTime() {
    return mCursorVisibleSeconds;
}
} // namespace Fast
