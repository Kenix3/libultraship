#include "libultraship/bridge/windowbridge.h"
#include "ship/window/Window.h"

static std::shared_ptr<Ship::Window> sWindow;

void WindowSetWindowComponent(std::shared_ptr<Ship::Window> window) {
    sWindow = std::move(window);
}

std::shared_ptr<Ship::Window> WindowGetWindowComponent() {
    return sWindow;
}

extern "C" {

uint32_t WindowGetWidth() {
    auto window = WindowGetWindowComponent();
    return window ? window->GetWidth() : 0;
}

uint32_t WindowGetHeight() {
    auto window = WindowGetWindowComponent();
    return window ? window->GetHeight() : 0;
}

float WindowGetAspectRatio() {
    auto window = WindowGetWindowComponent();
    return window ? window->GetCurrentAspectRatio() : 0.0f;
}

bool WindowIsRunning() {
    auto window = WindowGetWindowComponent();
    return window ? window->IsRunning() : false;
}

int32_t WindowGetPosX() {
    auto window = WindowGetWindowComponent();
    return window ? window->GetPosX() : 0;
}

int32_t WindowGetPosY() {
    auto window = WindowGetWindowComponent();
    return window ? window->GetPosY() : 0;
}

bool WindowIsFullscreen() {
    auto window = WindowGetWindowComponent();
    return window ? window->IsFullscreen() : false;
}
}
