#include "ControllerInputMapping.h"

namespace Ship {
ControllerInputMapping::ControllerInputMapping(PhysicalDeviceType physicalDeviceType)
    : ControllerMapping(physicalDeviceType) {
}

ControllerInputMapping::~ControllerInputMapping() {
}

std::string ControllerInputMapping::GetPhysicalInputName() {
    return "Unknown";
}
} // namespace Ship
