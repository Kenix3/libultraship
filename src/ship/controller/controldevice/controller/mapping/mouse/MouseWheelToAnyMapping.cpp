#include "MouseWheelToAnyMapping.h"
#include "Context.h"

#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"

namespace Ship {
MouseWheelToAnyMapping::MouseWheelToAnyMapping(WheelDirection wheelDirection)
    : ControllerInputMapping(PhysicalDeviceType::Mouse), mWheelDirection(wheelDirection) {
}

MouseWheelToAnyMapping::~MouseWheelToAnyMapping() {
}

std::string MouseWheelToAnyMapping::GetPhysicalInputName() {
    return wheelDirectionNames[static_cast<int>(mWheelDirection)];
}

std::string MouseWheelToAnyMapping::GetPhysicalDeviceName() {
    return "Mouse";
}
} // namespace Ship
