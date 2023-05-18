#include "core/bridge/windowbridge.h"
#include "window/Window.h"
#include "core/Context.h"

extern "C" {

uint32_t GetWindowWidth() {
    return LUS::Context::GetInstance()->GetWindow()->GetCurrentWidth();
}

uint32_t GetWindowHeight() {
    return LUS::Context::GetInstance()->GetWindow()->GetCurrentHeight();
}

float GetWindowAspectRatio() {
    return LUS::Context::GetInstance()->GetWindow()->GetCurrentAspectRatio();
}

void GetPixelDepthPrepare(float x, float y) {
    return LUS::Context::GetInstance()->GetWindow()->GetPixelDepthPrepare(x, y);
}

uint16_t GetPixelDepth(float x, float y) {
    return LUS::Context::GetInstance()->GetWindow()->GetPixelDepth(x, y);
}
}
