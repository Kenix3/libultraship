#ifdef __WIIU__
#include "WiiUButtonToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

#define MAX_SDL_RANGE (float)INT16_MAX

namespace LUS {
WiiUButtonToAxisDirectionMapping::WiiUButtonToAxisDirectionMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex,
                                                                 Stick stick, Direction direction,
                                                                 bool isNunchuk, bool isClassic, uint32_t wiiuControllerButton)
    : ControllerInputMapping(lusDeviceIndex),
      ControllerAxisDirectionMapping(lusDeviceIndex, portIndex, stick, direction),
      WiiUButtonToAnyMapping(lusDeviceIndex, isNunchuk, isClassic, wiiuControllerButton) {
}

float SDLButtonToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (!Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked() &&
        PhysicalButtonIsPressed()) {
        return MAX_AXIS_RANGE;
    }

    return 0.0f;
}

std::string SDLButtonToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-GP%d-SDLB%d", mPortIndex, mStick, mDirection,
                                 ControllerInputMapping::mLUSDeviceIndex, mControllerButton);
}

void SDLButtonToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarSetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                  "SDLButtonToAxisDirectionMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStick);
    CVarSetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    CVarSetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerInputMapping::mLUSDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), mControllerButton);
    CVarSave();
}

void SDLButtonToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarClear(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str());
    CVarSave();
}

uint8_t SDLButtonToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}
} // namespace LUS
#endif
