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

bool WindowIsRunning() {
    return Ship::Context::GetInstance()->GetWindow()->IsRunning();
}

int32_t WindowGetPosX() {
    return Ship::Context::GetInstance()->GetWindow()->GetPosX();
}

int32_t WindowGetPosY() {
    return Ship::Context::GetInstance()->GetWindow()->GetPosY();
}

bool WindowIsFullscreen() {
    return Ship::Context::GetInstance()->GetWindow()->IsFullscreen();
}
}
