#include "SDLAxisDirectionToButtonMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"

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
    switch (mControllerAxis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
            return StringHelper::Sprintf("Left Stick %s", mAxisDirection == NEGATIVE ? ICON_FA_ARROW_LEFT : ICON_FA_ARROW_RIGHT);
        case SDL_CONTROLLER_AXIS_LEFTY:
            return StringHelper::Sprintf("Left Stick %s", mAxisDirection == NEGATIVE ? ICON_FA_ARROW_UP : ICON_FA_ARROW_DOWN);
        case SDL_CONTROLLER_AXIS_RIGHTX:
            return StringHelper::Sprintf("Right Stick %s", mAxisDirection == NEGATIVE ? ICON_FA_ARROW_LEFT : ICON_FA_ARROW_RIGHT);
        case SDL_CONTROLLER_AXIS_RIGHTY:
            return StringHelper::Sprintf("Right Stick %s", mAxisDirection == NEGATIVE ? ICON_FA_ARROW_UP : ICON_FA_ARROW_DOWN);
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
            if (UsesPlaystationLayout()) {
                return "L2";
            }
            if (UsesSwitchLayout()) {
                return "LR";
            }
            if (UsesXboxLayout()) {
                return "LT";
            }
            break;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            if (UsesPlaystationLayout()) {
                return "R2";
            }
            if (UsesSwitchLayout()) {
                return "ZR";
            }
            if (UsesXboxLayout()) {
                return "RT";
            }
            break;
    }

    return StringHelper::Sprintf("Axis %d %s", mControllerAxis, mAxisDirection == NEGATIVE ? ICON_FA_MINUS : ICON_FA_PLUS);
}
} // namespace LUS
