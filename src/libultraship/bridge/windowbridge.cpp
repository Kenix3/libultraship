#include "libultraship/bridge/windowbridge.h"
#include "ship/window/Window.h"

static std::shared_ptr<Ship::Window> sWindow;

void WindowSetWindowComponent(std::shared_ptr<Ship::Window> window) {
    sWindow = std::move(window);
}

std::shared_ptr<Ship::Window> WindowGetWindowComponent() {
    return sWindow;
}

static Ship::Window* GetWindow() {
    return sWindow.get();
}

extern "C" {

uint32_t WindowGetWidth() {
    return GetWindow()->GetWidth();
}

uint32_t WindowGetHeight() {
    return GetWindow()->GetHeight();
}

float WindowGetAspectRatio() {
    return GetWindow()->GetCurrentAspectRatio();
}

bool WindowIsRunning() {
    return GetWindow()->IsRunning();
}

int32_t WindowGetPosX() {
    return GetWindow()->GetPosX();
}

int32_t WindowGetPosY() {
    return GetWindow()->GetPosY();
}

bool WindowIsFullscreen() {
    return GetWindow()->IsFullscreen();
}
}
