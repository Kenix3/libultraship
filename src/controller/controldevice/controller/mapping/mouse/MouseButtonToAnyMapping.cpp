#include "MouseButtonToAnyMapping.h"
#include "Context.h"

#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"

namespace Ship {
MouseButtonToAnyMapping::MouseButtonToAnyMapping(MouseBtn button)
    : ControllerInputMapping(ShipDeviceIndex::Mouse), mButton(button), mKeyPressed(false) {
}

MouseButtonToAnyMapping::~MouseButtonToAnyMapping() {
}

std::string MouseButtonToAnyMapping::GetPhysicalInputName() {
    return mouseBtnNames[static_cast<int>(mButton)];
}

bool MouseButtonToAnyMapping::ProcessMouseEvent(bool isPressed, MouseBtn button) {
    if (mButton != button) {
        return false;
    }

    mKeyPressed = isPressed;
    return true;
}

std::string MouseButtonToAnyMapping::GetPhysicalDeviceName() {
    return "Mouse";
}

bool MouseButtonToAnyMapping::PhysicalDeviceIsConnected() {
    return true;
}
} // namespace Ship
