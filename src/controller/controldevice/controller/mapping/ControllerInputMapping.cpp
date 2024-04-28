#include "ControllerInputMapping.h"

namespace Ship {
ControllerInputMapping::ControllerInputMapping(ShipDeviceIndex shipDeviceIndex) : ControllerMapping(shipDeviceIndex) {
}

ControllerInputMapping::~ControllerInputMapping() {
}

std::string ControllerInputMapping::GetPhysicalInputName() {
    return "Unknown";
}
} // namespace Ship
