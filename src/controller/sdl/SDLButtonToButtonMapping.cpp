#include "SDLButtonToButtonMapping.h"
#include <spdlog/spdlog.h>

namespace LUS {
SDLButtonToButtonMapping::SDLButtonToButtonMapping(uint16_t bitmask, int32_t sdlControllerIndex,
                                                   int32_t sdlControllerButton)
    : ButtonMapping(bitmask), SDLMapping(sdlControllerIndex) {
    mControllerButton = static_cast<SDL_GameControllerButton>(sdlControllerButton);
}

void SDLButtonToButtonMapping::UpdatePad(uint16_t& padButtons) {
    if (!ControllerLoaded()) {
        return;
    }

    if (SDL_GameControllerGetButton(mController, mControllerButton)) {
        padButtons |= mBitmask;
    }
}
} // namespace LUS
