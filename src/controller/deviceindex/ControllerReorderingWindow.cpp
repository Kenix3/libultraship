#include "ControllerReorderingWindow.h"
#include "utils/StringHelper.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include "Context.h"

namespace Ship {

ControllerReorderingWindow::~ControllerReorderingWindow() {
}

void ControllerReorderingWindow::InitElement() {
    mDeviceIndices.clear();
    mCurrentPortNumber = 1;
}

void ControllerReorderingWindow::UpdateElement() {
}

int32_t ControllerReorderingWindow::GetSDLIndexFromSDLInput() {
    int32_t sdlDeviceIndex = -1;

    std::unordered_map<int32_t, SDL_GameController*> sdlControllers;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            sdlControllers[i] = SDL_GameControllerOpen(i);
        }
    }

    for (auto [controllerIndex, controller] : sdlControllers) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) {
                sdlDeviceIndex = controllerIndex;
                break;
            }
        }

        if (sdlDeviceIndex != -1) {
            break;
        }

        for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
            const auto axis = static_cast<SDL_GameControllerAxis>(i);
            const auto axisValue = SDL_GameControllerGetAxis(controller, axis) / 32767.0f;
            if (axisValue < -0.7f || axisValue > 0.7f) {
                sdlDeviceIndex = controllerIndex;
                break;
            }
        }
    }

    for (auto [i, controller] : sdlControllers) {
        SDL_GameControllerClose(controller);
    }

    return sdlDeviceIndex;
}

void ControllerReorderingWindow::DrawElement() {
    Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->InitializeMappingsSinglePlayer();
    Hide();
    return;
}
} // namespace Ship