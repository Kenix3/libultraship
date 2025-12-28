#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardKeyToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include "ship/utils/StringHelper.h"
#include "ship/window/gui/IconsFontAwesome4.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/Context.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {
KeyboardKeyToAxisDirectionMapping::KeyboardKeyToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex,
                                                                     Direction direction, KbScancode scancode)
    : ControllerInputMapping(PhysicalDeviceType::Keyboard), KeyboardKeyToAnyMapping(scancode),
      ControllerAxisDirectionMapping(PhysicalDeviceType::Keyboard, portIndex, stickIndex, direction) {
}

float KeyboardKeyToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (Context::GetInstance()->GetControlDeck()->KeyboardGameInputBlocked()) {
        return 0.0f;
    }

    return mKeyPressed ? MAX_AXIS_RANGE : 0.0f;
}

std::string KeyboardKeyToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-KB%d", mPortIndex, mStickIndex, mDirection, mKeyboardScancode);
}

void KeyboardKeyToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    Ship::Context::GetInstance()->GetConsoleVariables()->SetString(
        StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
        "KeyboardKeyToAxisDirectionMapping");
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStickIndex);
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str(), mKeyboardScancode);
    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}

void KeyboardKeyToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}

int8_t KeyboardKeyToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_KEYBOARD;
}

std::string KeyboardKeyToAxisDirectionMapping::GetPhysicalDeviceName() {
    return KeyboardKeyToAnyMapping::GetPhysicalDeviceName();
}

std::string KeyboardKeyToAxisDirectionMapping::GetPhysicalInputName() {
    return KeyboardKeyToAnyMapping::GetPhysicalInputName();
}
} // namespace Ship
