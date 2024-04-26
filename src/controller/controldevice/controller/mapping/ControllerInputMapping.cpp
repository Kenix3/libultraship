#include "ControllerInputMapping.h"

namespace ShipDK {
ControllerInputMapping::ControllerInputMapping(ShipDKDeviceIndex shipDKDeviceIndex)
    : ControllerMapping(shipDKDeviceIndex) {
}

ControllerInputMapping::~ControllerInputMapping() {
}

std::string ControllerInputMapping::GetPhysicalInputName() {
    return "Unknown";
}
} // namespace ShipDK
