#include "ship/window/Window.h"
#include <string>
#include <fstream>
#include <iostream>
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "ship/Context.h"
#include "ship/controller/controldeck/ControlDeck.h"

#ifdef __APPLE__
#include "ship/utils/AppleFolderManager.h"
#endif

namespace Ship {

Window::Window(std::shared_ptr<Gui> gui, std::shared_ptr<MouseStateManager> mouseStateManager) {
    mGui = gui;
    mMouseStateManager = mouseStateManager;
    mAvailableWindowBackends = std::make_shared<std::vector<int32_t>>();
    mConfig = Context::GetInstance()->GetConfig();
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

int32_t Window::GetWindowBackend() {
    return mWindowBackend;
}

std::shared_ptr<std::vector<int32_t>> Window::GetAvailableWindowBackends() {
    return mAvailableWindowBackends;
}

bool Window::IsAvailableWindowBackend(int32_t backendId) {
    if (backendId < 0) {
        return false;
    }

    return std::find(mAvailableWindowBackends->begin(), mAvailableWindowBackends->end(), backendId) !=
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

void Window::SetWindowBackend(int32_t backend) {
    mWindowBackend = backend;
    mConfig->SetInt("Window.Backend.Id", GetWindowBackend());
    mConfig->SetString("Window.Backend.Name", GetWindowBackendName());
    mConfig->Save();
}

void Window::AddAvailableWindowBackend(int32_t backend) {
    mAvailableWindowBackends->push_back(backend);
}

int32_t Window::GetSavedWindowBackend() {
    auto backendId = mConfig->GetInt("Window.Backend.Id", -1);
    if (IsAvailableWindowBackend(backendId)) {
        return backendId;
    }

    SPDLOG_TRACE(
        "Could not find available WindowBackend matching id from config file ({}). Returning default WindowBackend.",
        backendId);

    if (mAvailableWindowBackends && !mAvailableWindowBackends->empty()) {
        return mAvailableWindowBackends->front();
    }
    return -1;
}

std::string Window::GetWindowBackendName() {
    return "";
}
} // namespace Ship
