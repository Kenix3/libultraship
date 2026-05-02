#include "ship/window/Window.h"
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "ship/Context.h"
#include "ship/controller/controldeck/ControlDeck.h"

#ifdef __APPLE__
#include "ship/utils/AppleFolderManager.h"
#endif

namespace Ship {

Window::Window(std::shared_ptr<Gui> gui, std::shared_ptr<MouseStateManager> mouseStateManager) : Component("Window") {
    mGui = gui;
    mMouseStateManager = mouseStateManager;
    mAvailableWindowBackends = std::make_shared<std::vector<WindowBackend>>();
    GetChildren().Add(gui);
}

Window::Window(std::shared_ptr<Gui> gui) : Window(gui, std::make_shared<MouseStateManager>()) {
}

Window::Window(std::vector<std::shared_ptr<GuiWindow>> guiWindows) : Window(std::make_shared<Gui>(guiWindows)) {
}

Window::Window() : Window(std::vector<std::shared_ptr<GuiWindow>>()) {
}

Window::~Window() {
    mGui->ShutDownImGui(this);
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

bool Window::ShouldAutoCaptureMouse() {
    return mMouseStateManager->ShouldAutoCaptureMouse();
}

void Window::SetAutoCaptureMouse(bool capture) {
    mMouseStateManager->SetAutoCaptureMouse(capture);
}

bool Window::ShouldForceCursorVisibility() {
    return mMouseStateManager->ShouldForceCursorVisibility();
}

void Window::SetForceCursorVisibility(bool visible) {
    mMouseStateManager->SetForceCursorVisibility(visible);
}

int32_t Window::GetFullscreenScancode() {
    return mFullscreenScancode;
}

int32_t Window::GetMouseCaptureScancode() {
    return mMouseCaptureScancode;
}

void Window::SetFullscreenScancode(int32_t scancode) {
    mFullscreenScancode = scancode;
}

void Window::SetMouseCaptureScancode(int32_t scancode) {
    mMouseCaptureScancode = scancode;
}

std::shared_ptr<MouseStateManager> Window::GetMouseStateManager() {
    return mMouseStateManager;
}

void Window::SetWindowBackend(WindowBackend backend) {
    mWindowBackend = backend;
    mConfig->SetWindowBackend(GetWindowBackend());
    mConfig->Save();
}

void Window::AddAvailableWindowBackend(WindowBackend backend) {
    mAvailableWindowBackends->push_back(backend);
}

void Window::OnInit() {
    mConfig = Context::GetInstance()->GetChildren().GetFirst<Config>();
    if (!mConfig) {
        throw std::runtime_error("Window requires Config in the component hierarchy");
    }
    if (!mConfig->IsInitialized()) {
        throw std::runtime_error("Window::Init requires Config to be initialized before Window");
    }
}

std::shared_ptr<Config> Window::GetConfig() const {
    return mConfig;
}
} // namespace Ship
