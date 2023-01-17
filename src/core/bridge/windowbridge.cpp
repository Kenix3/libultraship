#include "core/bridge/windowbridge.h"
#include "core/Window.h"

extern "C" {

uint32_t GetWindowWidth() {
    return Ship::Window::GetInstance()->GetCurrentWidth();
}

uint32_t GetWindowHeight() {
    return Ship::Window::GetInstance()->GetCurrentHeight();
}

float GetWindowAspectRatio() {
    return Ship::Window::GetInstance()->GetCurrentAspectRatio();
}

void GetPixelDepthPrepare(float x, float y) {
    return Ship::Window::GetInstance()->GetPixelDepthPrepare(x, y);
}

uint16_t GetPixelDepth(float x, float y) {
    return Ship::Window::GetInstance()->GetPixelDepth(x, y);
}
}
