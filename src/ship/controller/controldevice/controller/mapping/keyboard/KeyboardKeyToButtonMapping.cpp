#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardKeyToButtonMapping.h"
#include <spdlog/spdlog.h>
#include "ship/utils/StringHelper.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/window/Window.h"

namespace Ship {
KeyboardKeyToButtonMapping::KeyboardKeyToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                       KbScancode scancode, std::shared_ptr<ControlDeck> controlDeck,
                                                       std::shared_ptr<Window> window,
                                                       std::shared_ptr<ConsoleVariable> consoleVariable)
    : ControllerInputMapping(PhysicalDeviceType::Keyboard), KeyboardKeyToAnyMapping(scancode, window),
      ControllerButtonMapping(PhysicalDeviceType::Keyboard, portIndex, bitmask, controlDeck) {
    mConsoleVariable = std::move(consoleVariable);
    mControlDeck = std::move(controlDeck);
}

void KeyboardKeyToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (mControlDeck->KeyboardGameInputBlocked()) {
        return;
    }

    if (!mKeyPressed) {
        return;
    }

    padButtons |= mBitmask;
}

int8_t KeyboardKeyToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_KEYBOARD;
}

std::string KeyboardKeyToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-KB%d", mPortIndex, mBitmask, mKeyboardScancode);
}

void KeyboardKeyToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    mConsoleVariable->SetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                                "KeyboardKeyToButtonMapping");
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str(),
                                 mKeyboardScancode);
    mConsoleVariable->Save();
}

void KeyboardKeyToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();

    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str());

    mConsoleVariable->Save();
}

std::string KeyboardKeyToButtonMapping::GetPhysicalDeviceName() {
    return KeyboardKeyToAnyMapping::GetPhysicalDeviceName();
}

std::string KeyboardKeyToButtonMapping::GetPhysicalInputName() {
    return KeyboardKeyToAnyMapping::GetPhysicalInputName();
}
} // namespace Ship
