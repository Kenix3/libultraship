#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardKeyToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include "ship/utils/StringHelper.h"
#include "ship/window/gui/IconsFontAwesome4.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/Context.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/config/Config.h"
#include "ship/window/Window.h"

namespace Ship {
KeyboardKeyToAxisDirectionMapping::KeyboardKeyToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex,
                                                                     Direction direction, KbScancode scancode,
                                                                     std::shared_ptr<ControlDeck> controlDeck,
                                                                     std::shared_ptr<Config> config)
    : ControllerInputMapping(PhysicalDeviceType::Keyboard),
      KeyboardKeyToAnyMapping(scancode, controlDeck ? controlDeck->GetWindow()
                                                    : Context::GetInstance()->GetChildren().GetFirst<Window>()),
      ControllerAxisDirectionMapping(PhysicalDeviceType::Keyboard, portIndex, stickIndex, direction, controlDeck,
                                     config) {
    mConsoleVariable = Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>();
    if (controlDeck) {
        mControlDeck = std::move(controlDeck);
    } else {
        mControlDeck = Context::GetInstance()->GetChildren().GetFirst<ControlDeck>();
    }
}

float KeyboardKeyToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (mControlDeck->KeyboardGameInputBlocked()) {
        return 0.0f;
    }

    return mKeyPressed ? MAX_AXIS_RANGE : 0.0f;
}

std::string KeyboardKeyToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-KB%d", mPortIndex, mStickIndex, mDirection, mKeyboardScancode);
}

void KeyboardKeyToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    mConsoleVariable->SetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                                "KeyboardKeyToAxisDirectionMapping");
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStickIndex);
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str(),
                                 mKeyboardScancode);
    mConsoleVariable->Save();
}

void KeyboardKeyToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(
        StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->Save();
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
