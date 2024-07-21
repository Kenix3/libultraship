#include "KeyboardKeyToAnyMapping.h"
#include "Context.h"

#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"

namespace Ship {
KeyboardKeyToAnyMapping::KeyboardKeyToAnyMapping(KbScancode scancode)
    : ControllerInputMapping(ShipDeviceIndex::Keyboard), mKeyboardScancode(scancode), mKeyPressed(false) {
}

KeyboardKeyToAnyMapping::~KeyboardKeyToAnyMapping() {
}

std::string KeyboardKeyToAnyMapping::GetPhysicalInputName() {
    return Context::GetInstance()->GetWindow()->GetKeyName(mKeyboardScancode);
}

bool KeyboardKeyToAnyMapping::ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode) {
    if (eventType == KbEventType::LUS_KB_EVENT_ALL_KEYS_UP) {
        mKeyPressed = false;
        return true;
    }

    if (mKeyboardScancode != scancode) {
        return false;
    }

    if (eventType == KbEventType::LUS_KB_EVENT_KEY_DOWN) {
        mKeyPressed = true;
        return true;
    }

    if (eventType == KbEventType::LUS_KB_EVENT_KEY_UP) {
        mKeyPressed = false;
        return true;
    }

    return false;
}

std::string KeyboardKeyToAnyMapping::GetPhysicalDeviceName() {
    return "Keyboard";
}

bool KeyboardKeyToAnyMapping::PhysicalDeviceIsConnected() {
    // todo: handle non-keyboard devices?
    return true;
}
} // namespace Ship
