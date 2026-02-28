#include "ship/window/MouseStateManager.h"

#include "ship/Context.h"
#include "ship/window/Window.h"

namespace Ship {
MouseStateManager::MouseStateManager() {
}

MouseStateManager::~MouseStateManager() {
}

void MouseStateManager::StartFrame() {
    CursorVisibilityTimeoutTick();
}

void MouseStateManager::CursorVisibilityTimeoutTick() {
    static Coords sPrevMousePos;

    std::shared_ptr<Window> wnd = Context::GetInstance()->GetWindow();
    if (ShouldForceCursorVisibility() || wnd->IsMouseCaptured()) {
        return;
    }

    Coords mousePos = wnd->GetMousePos();
    bool mouseMoved = abs(mousePos.x - sPrevMousePos.x) > 0 || abs(mousePos.y - sPrevMousePos.y) > 0;
    sPrevMousePos = mousePos;

    if (mouseMoved) {
        wnd->SetCursorVisibility(true);
        ResetCursorVisibilityTimer();
        return;
    }

    if (mCursorVisibleTicksCounter == 0) {
        wnd->SetCursorVisibility(false);
        mCursorVisibleTicksCounter = -1;
        return;
    }

    if (mCursorVisibleTicksCounter > 0) {
        mCursorVisibleTicksCounter--;
    }
}

bool MouseStateManager::ShouldAutoCaptureMouse() {
    return mAutoCaptureMouse;
}

void MouseStateManager::SetAutoCaptureMouse(bool capture) {
    mAutoCaptureMouse = capture;
}

bool MouseStateManager::ShouldForceCursorVisibility() {
    return mForceCursorVisibility;
}

void MouseStateManager::SetForceCursorVisibility(bool visible) {
    mForceCursorVisibility = visible;
}

void MouseStateManager::ToggleMouseCaptureOverride() {
    const std::shared_ptr<Window> window = Context::GetInstance()->GetWindow();
    window->SetMouseCapture(!window->IsMouseCaptured());
}

void MouseStateManager::UpdateMouseCapture() {
    const std::shared_ptr<Window> window = Context::GetInstance()->GetWindow();
    if (!window->GetGui()->GetMenuOrMenubarVisible()) {
        window->SetMouseCapture(ShouldAutoCaptureMouse());
    } else {
        window->SetMouseCapture(false);
        ResetCursorVisibilityTimer();
    }
}

void MouseStateManager::ResetCursorVisibilityTimer() {
    mCursorVisibleTicksCounter = mCursorVisibleTicks;
}

void MouseStateManager::SetCursorVisibilityTimeTicks(uint32_t ticks) {
    mCursorVisibleTicks = ticks;
}

uint32_t MouseStateManager::GetCursorVisibilityTimeTicks() {
    return mCursorVisibleTicks;
}
} // namespace Ship
