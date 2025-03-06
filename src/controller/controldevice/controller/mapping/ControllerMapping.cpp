#include "ControllerMapping.h"

namespace Ship {
ControllerMapping::ControllerMapping(PhysicalDeviceType physicalDeviceType) : mPhysicalDeviceType(physicalDeviceType) {
}

ControllerMapping::~ControllerMapping() {
}

std::string ControllerMapping::GetPhysicalDeviceName() {
    return "Unknown";
}

PhysicalDeviceType ControllerMapping::GetPhysicalDeviceType() {
    return mPhysicalDeviceType;
}
} // namespace Ship
