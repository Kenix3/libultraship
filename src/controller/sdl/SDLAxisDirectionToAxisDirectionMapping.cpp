#include "SDLAxisDirectionToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>

namespace LUS {
SDLAxisDirectionToAxisDirectionMapping::SDLAxisDirectionToAxisDirectionMapping(int32_t sdlControllerIndex, int32_t sdlControllerAxis, int32_t axisDirection) {
    mControllerAxis = static_cast<SDL_GameControllerAxis>(sdlControllerAxis);
    mAxisDirection = static_cast<AxisDirection>(axisDirection);
}

void SDLAxisDirectionToButtonMapping::UpdatePad(uint16_t& padButtons) {
    if (!ControllerLoaded()) {
        return;
    }

    const auto axisValue = SDL_GameControllerGetAxis(mController, mControllerAxis);
    auto axisMinValue = 7680.0f;

    if ((mAxisDirection == POSITIVE && axisValue > axisMinValue) ||
        (mAxisDirection == NEGATIVE && axisValue < -axisMinValue)) {
        padButtons |= mBitmask;
    }
}
} // namespace LUS
