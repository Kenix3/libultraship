#include "ControllerInputMapping.h"

namespace Ship {
ControllerInputMapping::ControllerInputMapping(ShipDeviceType shipDeviceType) : ControllerMapping(shipDeviceType) {
}

ControllerInputMapping::~ControllerInputMapping() {
}

std::string ControllerInputMapping::GetPhysicalInputName() {
    return "Unknown";
}
} // namespace Ship
