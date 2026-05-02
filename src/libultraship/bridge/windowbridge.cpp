#include "libultraship/bridge/windowbridge.h"
#include "ship/window/Window.h"
#include "ship/Context.h"

extern "C" {

uint32_t WindowGetWidth() {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Window>()->GetWidth();
}

uint32_t WindowGetHeight() {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Window>()->GetHeight();
}

float WindowGetAspectRatio() {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Window>()->GetCurrentAspectRatio();
}

bool WindowIsRunning() {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Window>()->IsRunning();
}

int32_t WindowGetPosX() {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Window>()->GetPosX();
}

int32_t WindowGetPosY() {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Window>()->GetPosY();
}

bool WindowIsFullscreen() {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Window>()->IsFullscreen();
}
}
