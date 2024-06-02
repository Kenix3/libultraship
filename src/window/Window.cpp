#include "window/Window.h"
#include <string>
#include <fstream>
#include <iostream>
#include "public/bridge/consolevariablebridge.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "Context.h"

#ifdef __APPLE__
#include "utils/AppleFolderManager.h"
#endif

namespace Ship {

Window::Window(std::vector<std::shared_ptr<GuiWindow>> guiWindows) {
    mGui = std::make_shared<Gui>(guiWindows);
    mAvailableWindowBackends = std::make_shared<std::vector<WindowBackend>>();
    mConfig = Context::GetInstance()->GetConfig();
}

Window::Window() : Window(std::vector<std::shared_ptr<GuiWindow>>()) {
}

Window::~Window() {
    SPDLOG_DEBUG("destruct window");
}

void Window::ToggleFullscreen() {
    SetFullscreen(!IsFullscreen());
}

float Window::GetCurrentAspectRatio() {
    return (float)GetWidth() / (float)GetHeight();
}

int32_t Window::GetLastScancode() {
    return mLastScancode;
}

void Window::SetLastScancode(int32_t scanCode) {
    mLastScancode = scanCode;
}

std::shared_ptr<Gui> Window::GetGui() {
    return mGui;
}

void Window::SaveWindowToConfig() {
    // This accepts conf in because it can be run in the destruction of LUS.
    mConfig->SetBool("Window.Fullscreen.Enabled", IsFullscreen());
    if (IsFullscreen()) {
        mConfig->SetInt("Window.Fullscreen.Width", (int32_t)GetWidth());
        mConfig->SetInt("Window.Fullscreen.Height", (int32_t)GetHeight());
    } else {
        mConfig->SetInt("Window.Width", (int32_t)GetWidth());
        mConfig->SetInt("Window.Height", (int32_t)GetHeight());
        mConfig->SetInt("Window.PositionX", GetPosX());
        mConfig->SetInt("Window.PositionY", GetPosY());
    }
}

WindowBackend Window::GetWindowBackend() {
    return mWindowBackend;
}

std::shared_ptr<std::vector<WindowBackend>> Window::GetAvailableWindowBackends() {
    return mAvailableWindowBackends;
}

bool Window::IsAvailableWindowBackend(int32_t backendId) {
    // Verify the id is a valid backend enum value
    if (backendId < 0 || backendId >= static_cast<int>(WindowBackend::WINDOW_BACKEND_COUNT)) {
        return false;
    }

    // Verify the backend is available
    auto backend = static_cast<WindowBackend>(backendId);
    return std::find(mAvailableWindowBackends->begin(), mAvailableWindowBackends->end(), backend) !=
           mAvailableWindowBackends->end();
}

void Window::SetWindowBackend(WindowBackend backend) {
    mWindowBackend = backend;
    Ship::Context::GetInstance()->GetConfig()->SetWindowBackend(GetWindowBackend());
    Ship::Context::GetInstance()->GetConfig()->Save();
}

void Window::AddAvailableWindowBackend(WindowBackend backend) {
    mAvailableWindowBackends->push_back(backend);
}
} // namespace Ship
