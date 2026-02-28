#include <memory>

#include "ship/Context.h"
#include "fast/FastMouseStateManager.h"
#include "fast/Fast3dWindow.h"

namespace Fast {
void FastMouseStateManager::ResetCursorVisibilityTimer() {
    auto wnd = std::dynamic_pointer_cast<Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
    SetCursorVisibilityTimeTicks(mCursorVisibleSeconds * wnd->GetTargetFps());
    MouseStateManager::ResetCursorVisibilityTimer();
}

void FastMouseStateManager::SetCursorVisibilityTime(uint32_t seconds) {
    mCursorVisibleSeconds = seconds;
}

uint32_t FastMouseStateManager::GetCursorVisibilityTime() {
    return mCursorVisibleSeconds;
}
} // namespace Fast
