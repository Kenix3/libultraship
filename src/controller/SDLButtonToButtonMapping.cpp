#include "SDLButtonToButtonMapping.h"
#include <spdlog/spdlog.h>

namespace LUS {
SDLButtonToButtonMapping::SDLButtonToButtonMapping(uint16_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerButton)
    : ButtonMapping(bitmask), SDLMapping(sdlControllerIndex) {
    mControllerButton = static_cast<SDL_GameControllerButton>(sdlControllerButton);
}

void SDLButtonToButtonMapping::UpdatePad(uint16_t& padButtons) {
    SDL_GameControllerUpdate();

    // If the controller is disconnected, close it.
    if (mController != nullptr && !SDL_GameControllerGetAttached(mController)) {
        CloseController();
    }

    // Attempt to load the controller if it's not loaded
    if (mController == nullptr) {
        // If we failed to load the controller, don't process it.
        if (!OpenController()) {
            return;
        }
    }

    if (SDL_GameControllerGetButton(mController, mControllerButton)) {
        padButtons |= mBitmask;
    }
}
} // namespace LUS
