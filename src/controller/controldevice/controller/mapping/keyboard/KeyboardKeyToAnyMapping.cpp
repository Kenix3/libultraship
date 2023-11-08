#include "KeyboardKeyToAnyMapping.h"
#include "Context.h"

#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"

namespace LUS {
KeyboardKeyToAnyMapping::KeyboardKeyToAnyMapping(KbScancode scancode)
    : ControllerInputMapping(LUSDeviceIndex::Keyboard), mKeyboardScancode(scancode), mKeyPressed(false) {
}

KeyboardKeyToAnyMapping::~KeyboardKeyToAnyMapping() {
}

std::string KeyboardKeyToAnyMapping::GetPhysicalInputName() {
    return Context::GetInstance()->GetWindow()->GetKeyName(mKeyboardScancode);
}

bool KeyboardKeyToAnyMapping::ProcessKeyboardEvent(LUS::KbEventType eventType, LUS::KbScancode scancode) {
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
} // namespace LUS
