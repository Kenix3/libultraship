#include "SDLAxisDirectionToButtonMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"

namespace LUS {
SDLAxisDirectionToButtonMapping::SDLAxisDirectionToButtonMapping(uint8_t portIndex, uint16_t bitmask, int32_t sdlControllerIndex,
                                                                 int32_t sdlControllerAxis, int32_t axisDirection)
    : ControllerButtonMapping(portIndex, bitmask), SDLMapping(sdlControllerIndex) {
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

std::string SDLAxisDirectionToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-SDLI%d-SDLA%d-AD%s", mPortIndex, mBitmask, mControllerIndex, mControllerAxis, mAxisDirection == 1 ? "P" : "N");
}

void SDLAxisDirectionToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + GetButtonMappingId();
    CVarSetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                  "SDLAxisDirectionToButtonMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    CVarSetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), mControllerIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), mControllerAxis);
    CVarSetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), mAxisDirection);
    CVarSave();
}

void SDLAxisDirectionToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + GetButtonMappingId();
    CVarClear(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str());
    CVarSave();
}

std::string SDLAxisDirectionToButtonMapping::GetButtonName() {
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
} // namespace LUS
