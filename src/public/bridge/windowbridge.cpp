#include "public/bridge/windowbridge.h"
#include "window/Window.h"
#include "Context.h"

extern "C" {

uint32_t WindowGetWidth() {
    return Ship::Context::GetInstance()->GetWindow()->GetWidth();
}

uint32_t WindowGetHeight() {
    return Ship::Context::GetInstance()->GetWindow()->GetHeight();
}

float WindowGetAspectRatio() {
    return Ship::Context::GetInstance()->GetWindow()->GetCurrentAspectRatio();
}

void WindowGetPixelDepthPrepare(float x, float y) {
    return Ship::Context::GetInstance()->GetWindow()->GetPixelDepthPrepare(x, y);
}

uint16_t WindowGetPixelDepth(float x, float y) {
    return Ship::Context::GetInstance()->GetWindow()->GetPixelDepth(x, y);
}
}
