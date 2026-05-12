#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardKeyToAnyMapping.h"

#include "ship/utils/StringHelper.h"
#include "ship/window/gui/IconsFontAwesome4.h"
#include "ship/window/Window.h"
#include <stdexcept>

namespace Ship {
KeyboardKeyToAnyMapping::KeyboardKeyToAnyMapping(KbScancode scancode, std::shared_ptr<Window> window)
    : ControllerInputMapping(PhysicalDeviceType::Keyboard), mKeyboardScancode(scancode), mKeyPressed(false),
      mWindow(std::move(window)) {
}

KeyboardKeyToAnyMapping::~KeyboardKeyToAnyMapping() {
}

std::string KeyboardKeyToAnyMapping::GetPhysicalInputName() {
    return GetWindow()->GetKeyName(mKeyboardScancode);
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

std::shared_ptr<Window> KeyboardKeyToAnyMapping::GetWindow() const {
    if (!mWindow) {
        throw std::runtime_error("KeyboardKeyToAnyMapping requires Window dependency");
    }
    if (!mWindow->IsInitialized()) {
        throw std::runtime_error("KeyboardKeyToAnyMapping requires Window to be initialized");
    }
    return mWindow;
}
} // namespace Ship
