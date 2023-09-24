#include "SDLAxisDirectionToButtonMapping.h"
#include <spdlog/spdlog.h>

namespace LUS {
SDLAxisDirectionToButtonMapping::SDLAxisDirectionToButtonMapping(uint16_t bitmask, int32_t sdlControllerIndex,
                                                                 int32_t sdlControllerAxis, int32_t axisDirection)
    : ButtonMapping(bitmask), SDLMapping(sdlControllerIndex) {
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

uint8_t SDLAxisDirectionToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}

std::string SDLAxisDirectionToButtonMapping::GetButtonName() {
    auto blarg = mController;
    return "blarg";
}
} // namespace LUS
