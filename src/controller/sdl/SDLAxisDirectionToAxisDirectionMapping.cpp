#include "SDLAxisDirectionToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"

#define MAX_SDL_RANGE (float)INT16_MAX

namespace LUS {
SDLAxisDirectionToAxisDirectionMapping::SDLAxisDirectionToAxisDirectionMapping(int32_t sdlControllerIndex,
                                                                               int32_t sdlControllerAxis,
                                                                               int32_t axisDirection)
    : SDLMapping(sdlControllerIndex) {
    mControllerAxis = static_cast<SDL_GameControllerAxis>(sdlControllerAxis);
    mAxisDirection = static_cast<AxisDirection>(axisDirection);
}

SDLAxisDirectionToAxisDirectionMapping::SDLAxisDirectionToAxisDirectionMapping(std::string uuid, int32_t sdlControllerIndex,
                                                                               int32_t sdlControllerAxis,
                                                                               int32_t axisDirection)
    : SDLMapping(sdlControllerIndex), AxisDirectionMapping(uuid) {
    mControllerAxis = static_cast<SDL_GameControllerAxis>(sdlControllerAxis);
    mAxisDirection = static_cast<AxisDirection>(axisDirection);
}

float SDLAxisDirectionToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (!ControllerLoaded()) {
        return 0.0f;
    }

    const auto axisValue = SDL_GameControllerGetAxis(mController, mControllerAxis);

    if ((mAxisDirection == POSITIVE && axisValue < 0) || (mAxisDirection == NEGATIVE && axisValue > 0)) {
        return 0.0f;
    }

    // scale {-32768 ... +32767} to {-MAX_AXIS_RANGE ... +MAX_AXIS_RANGE}
    // and return the absolute value of it
    return fabs(axisValue * MAX_AXIS_RANGE / MAX_SDL_RANGE);
}

std::string SDLAxisDirectionToAxisDirectionMapping::GetAxisDirectionName() {
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

uint8_t SDLAxisDirectionToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}
} // namespace LUS
