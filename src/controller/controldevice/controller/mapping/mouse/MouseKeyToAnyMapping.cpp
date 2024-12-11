#include "MouseKeyToAnyMapping.h"
#include "Context.h"

#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"

namespace Ship {
MouseKeyToAnyMapping::MouseKeyToAnyMapping(MouseBtn button) // TODO: should it have separate device index?
    : ControllerInputMapping(ShipDeviceIndex::Keyboard), mButton(button), mKeyPressed(false) {
}

MouseKeyToAnyMapping::~MouseKeyToAnyMapping() {
}

std::string MouseKeyToAnyMapping::GetPhysicalInputName() {
    using MouseBtn;
    switch (mButton) {
        case LEFT:
            return "Left";
        case MIDDLE:
            return "Middle";
        case RIGHT:
            return "Right";
        case BACKWARD:
            return "Backward";
        case FORWARD:
            return "Forward";
    }
}

bool MouseKeyToAnyMapping::ProcessMouseEvent(bool isPressed, MouseBtn button) {
    if (mButton != button) {
        return false;
    }

    mKeyPressed = isPressed;
    return true;
}

std::string MouseKeyToAnyMapping::GetPhysicalDeviceName() {
    return "Mouse";
}

bool MouseKeyToAnyMapping::PhysicalDeviceIsConnected() {
    return true;
}
} // namespace Ship
