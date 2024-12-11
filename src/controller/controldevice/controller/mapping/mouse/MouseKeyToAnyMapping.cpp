#include "MouseKeyToAnyMapping.h"
#include "Context.h"

#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"

namespace Ship {
MouseKeyToAnyMapping::MouseKeyToAnyMapping(MouseBtn button)
    : ControllerInputMapping(ShipDeviceIndex::Mouse), mButton(button), mKeyPressed(false) {
}

MouseKeyToAnyMapping::~MouseKeyToAnyMapping() {
}

std::string MouseKeyToAnyMapping::GetPhysicalInputName() {
    switch (mButton) {
        case MouseBtn::LEFT:
            return "Left";
        case MouseBtn::MIDDLE:
            return "Middle";
        case MouseBtn::RIGHT:
            return "Right";
        case MouseBtn::BACKWARD:
            return "Backward";
        case MouseBtn::FORWARD:
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
