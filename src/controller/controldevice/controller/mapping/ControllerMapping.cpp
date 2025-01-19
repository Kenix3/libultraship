#include "ControllerMapping.h"

namespace Ship {
ControllerMapping::ControllerMapping(ShipDeviceType shipDeviceType) : mShipDeviceType(shipDeviceType) {
}

ControllerMapping::~ControllerMapping() {
}

std::string ControllerMapping::GetPhysicalDeviceName() {
    return "Unknown";
}

ShipDeviceType ControllerMapping::GetShipDeviceType() {
    return mShipDeviceType;
}
} // namespace Ship
