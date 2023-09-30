#include "ControllerMapping.h"

namespace LUS {
ControllerMapping::ControllerMapping() {
}

ControllerMapping::~ControllerMapping() {
}

std::string ControllerMapping::GetPhysicalDeviceName() {
    return "Unknown";
}

int32_t ControllerMapping::GetPhysicalDeviceIndex() {
    return -1;
}
} // namespace LUS
