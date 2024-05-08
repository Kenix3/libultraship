#include "public/bridge/windowbridge.h"
#include "window/Window.h"
#include "Context.h"

extern "C" {

uint32_t WindowGetWidth(void) {
    return Ship::Context::GetInstance()->GetWindow()->GetWidth();
}

uint32_t WindowGetHeight(void) {
    return Ship::Context::GetInstance()->GetWindow()->GetHeight();
}

float WindowGetAspectRatio(void) {
    return Ship::Context::GetInstance()->GetWindow()->GetCurrentAspectRatio();
}

bool WindowIsRunning(void) {
    return Ship::Context::GetInstance()->GetWindow()->IsRunning();
}

int32_t WindowGetPosX(void) {
    return Ship::Context::GetInstance()->GetWindow()->GetPosX();
}

int32_t WindowGetPosY(void) {
    return Ship::Context::GetInstance()->GetWindow()->GetPosY();
}

bool WindowIsFullscreen(void) {
    return Ship::Context::GetInstance()->GetWindow()->IsFullscreen();
}
}
