#include "SDLMapping.h"
#include <spdlog/spdlog.h>

namespace LUS {
SDLMapping::SDLMapping(int32_t sdlControllerIndex) : mControllerIndex(sdlControllerIndex), mController(nullptr) {
}

SDLMapping::~SDLMapping() {
}

bool SDLMapping::OpenController() {
    const auto newCont = SDL_GameControllerOpen(mControllerIndex);

    // We failed to load the controller. Go to next.
    if (newCont == nullptr) {
        return false;
    }

    mController = newCont;
    return true;
}

bool SDLMapping::CloseController() {
    if (mController != nullptr && SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        // if (CanRumble()) {
        //     SDL_GameControllerRumble(mController, 0, 0, 0);
        // }
        SDL_GameControllerClose(mController);
    }
    mController = nullptr;

    return true;
}

bool SDLMapping::ControllerLoaded() {
    SDL_GameControllerUpdate();

    // If the controller is disconnected, close it.
    if (mController != nullptr && !SDL_GameControllerGetAttached(mController)) {
        CloseController();
    }

    // Attempt to load the controller if it's not loaded
    if (mController == nullptr) {
        // If we failed to load the controller, don't process it.
        if (!OpenController()) {
            return false;
        }
    }

    return true;
}

SDL_GameControllerType SDLMapping::GetSDLControllerType() {
    return SDL_GameControllerGetType(mController);
}

bool SDLMapping::UsesPlaystationLayout() {
    auto type = GetSDLControllerType();
    return type == SDL_CONTROLLER_TYPE_PS3 || type == SDL_CONTROLLER_TYPE_PS4 || type == SDL_CONTROLLER_TYPE_PS5;
}

bool SDLMapping::UsesSwitchLayout() {
    auto type = GetSDLControllerType();
    return type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO || type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR;
}

bool SDLMapping::UsesXboxLayout() {
    auto type = GetSDLControllerType();
    return type == SDL_CONTROLLER_TYPE_XBOX360 || type == SDL_CONTROLLER_TYPE_XBOXONE;
}
} // namespace LUS
