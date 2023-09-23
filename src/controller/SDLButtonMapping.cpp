#include "SDLButtonMapping.h"
#include <spdlog/spdlog.h>

namespace LUS {
SDLButtonMapping::SDLButtonMapping(int32_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerButton)
    : ButtonMapping(bitmask), SDLMapping(sdlControllerIndex), mControllerButton(sdlControllerButton) {
}

void SDLButtonMapping::UpdatePad(int32_t& padButtons) {
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
    

}
} // namespace LUS