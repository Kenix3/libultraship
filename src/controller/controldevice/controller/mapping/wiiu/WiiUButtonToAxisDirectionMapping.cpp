#ifdef __WIIU__
#include "WiiUButtonToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

namespace LUS {
WiiUButtonToAxisDirectionMapping::WiiUButtonToAxisDirectionMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex,
                                                                   Stick stick, Direction direction, bool isNunchuk,
                                                                   bool isClassic, uint32_t wiiuControllerButton)
    : ControllerInputMapping(lusDeviceIndex),
      ControllerAxisDirectionMapping(lusDeviceIndex, portIndex, stick, direction),
      WiiUButtonToAnyMapping(lusDeviceIndex, isNunchuk, isClassic, wiiuControllerButton) {
}

float WiiUButtonToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (!Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked() && PhysicalButtonIsPressed()) {
        return MAX_AXIS_RANGE;
    }

    return 0.0f;
}

std::string WiiUButtonToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-LUSI%d-N%d-C%d-B%d", mPortIndex, mStick, mDirection,
                                 ControllerInputMapping::mLUSDeviceIndex, mIsNunchukButton, mIsClassicControllerButton,
                                 mControllerButton);
}

void WiiUButtonToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarSetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                  "WiiUButtonToAxisDirectionMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStick);
    CVarSetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    CVarSetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerInputMapping::mLUSDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.IsClassicControllerButton", mappingCvarKey.c_str()).c_str(),
                   mIsClassicControllerButton);
    CVarSetInteger(StringHelper::Sprintf("%s.IsNunchukButton", mappingCvarKey.c_str()).c_str(), mIsNunchukButton);
    CVarSetInteger(StringHelper::Sprintf("%s.WiiUControllerButton", mappingCvarKey.c_str()).c_str(), mControllerButton);
    CVarSave();
}

void WiiUButtonToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarClear(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.IsClassicControllerButton", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.IsNunchukButton", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.WiiUControllerButton", mappingCvarKey.c_str()).c_str());
    CVarSave();
}

uint8_t WiiUButtonToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}
} // namespace LUS
#endif
