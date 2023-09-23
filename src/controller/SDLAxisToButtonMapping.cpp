#include "SDLAxisToButtonMapping.h"
#include <spdlog/spdlog.h>

namespace LUS {
SDLAxisToButtonMapping::SDLAxisToButtonMapping(int32_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerAxis, int32_t axisDirection)
    : ButtonMapping(bitmask), SDLMapping(sdlControllerIndex) {
    mControllerAxis = static_cast<SDL_GameControllerAxis>(sdlControllerAxis);
    mAxisDirection = static_cast<AxisDirection>(axisDirection);
}

void SDLAxisToButtonMapping::UpdatePad(int32_t& padButtons) {
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

    const auto axisValue = SDL_GameControllerGetAxis(mController, mControllerAxis);
    auto axisMinValue = 7680.0f;

    if ((mAxisDirection == POSITIVE && axisValue > axisMinValue) ||
        (mAxisDirection == NEGATIVE && axisValue < -axisMinValue)) {
        padButtons |= mBitmask;
    }
}
} // namespace LUS
