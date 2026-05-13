#include "ship/window/MouseStateManager.h"

#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"

namespace Ship {
MouseStateManager::MouseStateManager() {
}

MouseStateManager::~MouseStateManager() {
}

void MouseStateManager::SetWindow(std::shared_ptr<Window> window) {
    mWindow = std::move(window);
}

void MouseStateManager::StartFrame() {
    CursorVisibilityTimeoutTick();
}

void MouseStateManager::CursorVisibilityTimeoutTick() {
    static Coords sPrevMousePos;

    if (ShouldForceCursorVisibility() || mWindow->IsMouseCaptured()) {
        return;
    }

    Coords mousePos = mWindow->GetMousePos();
    bool mouseMoved = abs(mousePos.x - sPrevMousePos.x) > 0 || abs(mousePos.y - sPrevMousePos.y) > 0;
    sPrevMousePos = mousePos;

    if (mouseMoved) {
        mWindow->SetCursorVisibility(true);
        ResetCursorVisibilityTimer();
        return;
    }

    if (mCursorVisibleTicksCounter == 0) {
        mWindow->SetCursorVisibility(false);
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
    mWindow->SetMouseCapture(!mWindow->IsMouseCaptured());
}

void MouseStateManager::UpdateMouseCapture() {
    if (!mWindow->GetGui()->GetMenuOrMenubarVisible()) {
        mWindow->SetMouseCapture(ShouldAutoCaptureMouse());
    } else {
        mWindow->SetMouseCapture(false);
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
