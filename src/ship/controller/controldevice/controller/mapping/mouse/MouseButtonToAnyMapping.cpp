#include "MouseButtonToAnyMapping.h"
#include "Context.h"

#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"

namespace Ship {
MouseButtonToAnyMapping::MouseButtonToAnyMapping(MouseBtn button)
    : ControllerInputMapping(PhysicalDeviceType::Mouse), mButton(button), mKeyPressed(false) {
}

MouseButtonToAnyMapping::~MouseButtonToAnyMapping() {
}

std::string MouseButtonToAnyMapping::GetPhysicalInputName() {
    return mouseBtnNames[static_cast<int>(mButton)];
}

bool MouseButtonToAnyMapping::ProcessMouseButtonEvent(bool isPressed, MouseBtn button) {
    if (mButton != button) {
        return false;
    }

    mKeyPressed = isPressed;
    return true;
}

std::string MouseButtonToAnyMapping::GetPhysicalDeviceName() {
    return "Mouse";
}
} // namespace Ship
