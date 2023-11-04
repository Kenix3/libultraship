#include "SDLAxisDirectionToButtonMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

namespace LUS {
SDLAxisDirectionToButtonMapping::SDLAxisDirectionToButtonMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex,
                                                                 uint16_t bitmask, int32_t sdlControllerAxis,
                                                                 int32_t axisDirection)
    : ControllerInputMapping(lusDeviceIndex), ControllerButtonMapping(lusDeviceIndex, portIndex, bitmask),
      SDLAxisDirectionToAnyMapping(lusDeviceIndex, sdlControllerAxis, axisDirection) {
}

void SDLAxisDirectionToButtonMapping::UpdatePad(uint16_t& padButtons) {
    if (!ControllerLoaded()) {
        return;
    }

    if (Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked()) {
        return;
    }

    const auto axisValue = SDL_GameControllerGetAxis(mController, mControllerAxis);
    int32_t axisThresholdPercentage = 25;
    auto indexMapping = Context::GetInstance()
                            ->GetControlDeck()
                            ->GetDeviceIndexMappingManager()
                            ->GetDeviceIndexMappingFromLUSDeviceIndex(ControllerInputMapping::mLUSDeviceIndex);
    auto sdlIndexMapping = std::dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(indexMapping);

    if (sdlIndexMapping != nullptr) {
        if (AxisIsStick()) {
            axisThresholdPercentage = sdlIndexMapping->GetStickAxisThresholdPercentage();
        } else if (AxisIsTrigger()) {
            axisThresholdPercentage = sdlIndexMapping->GetTriggerAxisThresholdPercentage();
        }
    }

    auto axisMinValue = SDL_JOYSTICK_AXIS_MAX * (axisThresholdPercentage / 100.0f);
    if ((mAxisDirection == POSITIVE && axisValue > axisMinValue) ||
        (mAxisDirection == NEGATIVE && axisValue < -axisMinValue)) {
        padButtons |= mBitmask;
    }
}

uint8_t SDLAxisDirectionToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}

std::string SDLAxisDirectionToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-LUSI%d-SDLA%d-AD%s", mPortIndex, mBitmask,
                                 ControllerInputMapping::mLUSDeviceIndex, mControllerAxis,
                                 mAxisDirection == 1 ? "P" : "N");
}

void SDLAxisDirectionToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + GetButtonMappingId();
    CVarSetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                  "SDLAxisDirectionToButtonMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    CVarSetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerInputMapping::mLUSDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), mControllerAxis);
    CVarSetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), mAxisDirection);
    CVarSave();
}

void SDLAxisDirectionToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + GetButtonMappingId();
    CVarClear(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str());
    CVarSave();
}
} // namespace LUS
