#include "libultraship/bridge/windowbridge.h"
#include "ship/window/Window.h"
#include "ship/Context.h"

static std::shared_ptr<Ship::Window> sWindow;

static Ship::Window* GetWindow() {
    if (!sWindow) {
        sWindow = Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Window>();
    }
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
