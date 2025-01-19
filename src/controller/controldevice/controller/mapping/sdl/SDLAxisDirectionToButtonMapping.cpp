#include "SDLAxisDirectionToButtonMapping.h"
#include <spdlog/spdlog.h>
#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

namespace Ship {
SDLAxisDirectionToButtonMapping::SDLAxisDirectionToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                                 int32_t sdlControllerAxis, int32_t axisDirection)
    : ControllerInputMapping(PhysicalDeviceType::SDLGamepad),
      ControllerButtonMapping(PhysicalDeviceType::SDLGamepad, portIndex, bitmask),
      SDLAxisDirectionToAnyMapping(sdlControllerAxis, axisDirection) {
}

void SDLAxisDirectionToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked()) {
        return;
    }

    int32_t axisThresholdPercentage = 25;
    if (AxisIsStick()) {
        axisThresholdPercentage = Ship::Context::GetInstance()
                                      ->GetControlDeck()
                                      ->GetGlobalSDLDeviceSettings()
                                      ->GetStickAxisThresholdPercentage();
    } else if (AxisIsTrigger()) {
        axisThresholdPercentage = Ship::Context::GetInstance()
                                      ->GetControlDeck()
                                      ->GetGlobalSDLDeviceSettings()
                                      ->GetTriggerAxisThresholdPercentage();
    }

    for (const auto& [instanceId, gamepad] :
         Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager()->GetConnectedSDLGamepadsForPort(
             mPortIndex)) {
        const auto axisValue = SDL_GameControllerGetAxis(gamepad, mControllerAxis);

        auto axisMinValue = SDL_JOYSTICK_AXIS_MAX * (axisThresholdPercentage / 100.0f);
        if ((mAxisDirection == POSITIVE && axisValue > axisMinValue) ||
            (mAxisDirection == NEGATIVE && axisValue < -axisMinValue)) {
            padButtons |= mBitmask;
        }
    }
}

int8_t SDLAxisDirectionToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}

std::string SDLAxisDirectionToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-SDLA%d-AD%s", mPortIndex, mBitmask, mControllerAxis,
                                 mAxisDirection == 1 ? "P" : "N");
}

void SDLAxisDirectionToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    CVarSetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                  "SDLAxisDirectionToButtonMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    CVarSetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), mControllerAxis);
    CVarSetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), mAxisDirection);
    CVarSave();
}

void SDLAxisDirectionToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    CVarClear(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str());
    CVarSave();
}
} // namespace Ship
