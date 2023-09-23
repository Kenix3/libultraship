#include "SDLButtonMapping.h"
#include <spdlog/spdlog.h>

namespace LUS {
SDLMapping::SDLMapping(int32_t sdlControllerIndex) : mControllerIndex(sdlControllerIndex) {
}

bool SDLMapping::OpenController() {
    const auto newCont = SDL_GameControllerOpen(mControllerIndex);

    // We failed to load the controller. Go to next.
    if (newCont == nullptr) {
        SPDLOG_ERROR("SDL Controller failed to open: ({})", SDL_GetError());
        return false;
    }

    mController = newCont;
    return true;
}

bool SDLMapping::CloseController() {
    if (mController != nullptr && SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        if (CanRumble()) {
            SDL_GameControllerRumble(mController, 0, 0, 0);
        }
        SDL_GameControllerClose(mController);
    }
    mController = nullptr;

    return true;
}
} // namespace LUS
