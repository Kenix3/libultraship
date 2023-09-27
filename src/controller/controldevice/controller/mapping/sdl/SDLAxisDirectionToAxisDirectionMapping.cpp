#include "SDLAxisDirectionToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"

#define MAX_SDL_RANGE (float)INT16_MAX

namespace LUS {
SDLAxisDirectionToAxisDirectionMapping::SDLAxisDirectionToAxisDirectionMapping(
    uint8_t portIndex, Stick stick, Direction direction, int32_t sdlControllerIndex,
                                                                               int32_t sdlControllerAxis,
                                                                               int32_t axisDirection)
    : ControllerAxisDirectionMapping(portIndex, stick, direction), SDLMapping(sdlControllerIndex) {
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

std::string SDLAxisDirectionToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-SDLI%d-SDLA%d-AD%s", mPortIndex, mStick, mDirection, mControllerIndex, mControllerAxis, mAxisDirection == 1 ? "P" : "N");
}

void SDLAxisDirectionToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarSetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                  "SDLAxisDirectionToAxisDirectionMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStick);
    CVarSetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);              
    CVarSetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), mControllerIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), mControllerAxis);
    CVarSetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), mAxisDirection);
    CVarSave();
}

void SDLAxisDirectionToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarClear(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str()); 
    CVarClear(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str());
    CVarSave();
}

std::string SDLAxisDirectionToAxisDirectionMapping::GetAxisDirectionName() {
    switch (mControllerAxis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
            return StringHelper::Sprintf("Left Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_LEFT : ICON_FA_ARROW_RIGHT);
        case SDL_CONTROLLER_AXIS_LEFTY:
            return StringHelper::Sprintf("Left Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_UP : ICON_FA_ARROW_DOWN);
        case SDL_CONTROLLER_AXIS_RIGHTX:
            return StringHelper::Sprintf("Right Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_LEFT : ICON_FA_ARROW_RIGHT);
        case SDL_CONTROLLER_AXIS_RIGHTY:
            return StringHelper::Sprintf("Right Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_UP : ICON_FA_ARROW_DOWN);
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

    return StringHelper::Sprintf("Axis %d %s", mControllerAxis,
                                 mAxisDirection == NEGATIVE ? ICON_FA_MINUS : ICON_FA_PLUS);
}

uint8_t SDLAxisDirectionToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}
} // namespace LUS
