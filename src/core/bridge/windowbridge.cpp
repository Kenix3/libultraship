#include "core/bridge/windowbridge.h"
#include "core/Window.h"
#include "core/Context.h"

extern "C" {

uint32_t GetWindowWidth() {
    return Ship::Context::GetInstance()->GetWindow()->GetCurrentWidth();
}

uint32_t GetWindowHeight() {
    return Ship::Context::GetInstance()->GetWindow()->GetCurrentHeight();
}

float GetWindowAspectRatio() {
    return Ship::Context::GetInstance()->GetWindow()->GetCurrentAspectRatio();
}

void GetPixelDepthPrepare(float x, float y) {
    return Ship::Context::GetInstance()->GetWindow()->GetPixelDepthPrepare(x, y);
}

uint16_t GetPixelDepth(float x, float y) {
    return Ship::Context::GetInstance()->GetWindow()->GetPixelDepth(x, y);
}
}
