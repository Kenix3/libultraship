#include "ControllerInputMapping.h"

namespace LUS {
ControllerInputMapping::ControllerInputMapping(LUSDeviceIndex lusDeviceIndex) : ControllerMapping(lusDeviceIndex) {
}

ControllerInputMapping::~ControllerInputMapping() {
}

std::string ControllerInputMapping::GetPhysicalInputName() {
    return "Unknown";
}
} // namespace LUS
