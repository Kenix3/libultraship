#include "KeyboardKeyToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"

namespace LUS {
KeyboardKeyToAxisDirectionMapping::KeyboardKeyToAxisDirectionMapping(
    uint8_t portIndex, Stick stick, Direction direction, KbScancode scancode)
    : ControllerAxisDirectionMapping(portIndex, stick, direction), KeyboardKeyToAnyMapping(scancode) {
}

float KeyboardKeyToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    return mKeyPressed ? MAX_AXIS_RANGE : 0.0f;
}

std::string KeyboardKeyToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-KB%d", mPortIndex, mStick, mDirection, mKeyboardScancode);
}

void KeyboardKeyToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarSetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                  "KeyboardKeyToAxisDirectionMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStick);
    CVarSetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);              
    CVarSetInteger(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str(), mKeyboardScancode);
    CVarSave();
}

void KeyboardKeyToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarClear(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str()); 
    CVarClear(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str());
    CVarSave();
}

uint8_t KeyboardKeyToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_KEYBOARD;
}
} // namespace LUS
