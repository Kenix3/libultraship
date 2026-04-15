#include "ship/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.h"
#include <spdlog/spdlog.h>
#include "ship/utils/StringHelper.h"
#include "ship/window/gui/IconsFontAwesome4.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/Context.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {
SDLAxisDirectionToButtonMapping::SDLAxisDirectionToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                                 int32_t sdlControllerAxis, int32_t axisDirection)
    : ControllerInputMapping(PhysicalDeviceType::SDLGamepad),
      ControllerButtonMapping(PhysicalDeviceType::SDLGamepad, portIndex, bitmask),
      SDLAxisDirectionToAnyMapping(sdlControllerAxis, axisDirection) {
}

void SDLAxisDirectionToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (Context::GetInstance()->GetChildren().GetFirst<ControlDeck>()->GamepadGameInputBlocked()) {
        return;
    }

    int32_t axisThresholdPercentage = 25;
    if (AxisIsStick()) {
        axisThresholdPercentage = Ship::Context::GetInstance()
                                      ->GetChildren()
                                      .GetFirst<ControlDeck>()
                                      ->GetGlobalSDLDeviceSettings()
                                      ->GetStickAxisThresholdPercentage();
    } else if (AxisIsTrigger()) {
        axisThresholdPercentage = Ship::Context::GetInstance()
                                      ->GetChildren()
                                      .GetFirst<ControlDeck>()
                                      ->GetGlobalSDLDeviceSettings()
                                      ->GetTriggerAxisThresholdPercentage();
    }

    for (const auto& [instanceId, gamepad] : Context::GetInstance()
                                                 ->GetChildren()
                                                 .GetFirst<ControlDeck>()
                                                 ->GetConnectedPhysicalDeviceManager()
                                                 ->GetConnectedSDLGamepadsForPort(mPortIndex)) {
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
    Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>()->SetString(
        StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
        "SDLAxisDirectionToButtonMapping");
    Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>()->SetInteger(
        StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>()->SetInteger(
        StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), mControllerAxis);
    Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>()->SetInteger(
        StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), mAxisDirection);
    Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>()->Save();
}

void SDLAxisDirectionToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>()->ClearVariable(
        StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>()->ClearVariable(
        StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>()->ClearVariable(
        StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>()->ClearVariable(
        StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>()->Save();
}

std::string SDLAxisDirectionToButtonMapping::GetPhysicalDeviceName() {
    return SDLAxisDirectionToAnyMapping::GetPhysicalDeviceName();
}

std::string SDLAxisDirectionToButtonMapping::GetPhysicalInputName() {
    return SDLAxisDirectionToAnyMapping::GetPhysicalInputName();
}
} // namespace Ship
