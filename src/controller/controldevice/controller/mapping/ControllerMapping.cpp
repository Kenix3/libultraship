#include "ControllerMapping.h"

namespace Ship {
ControllerMapping::ControllerMapping(ShipDeviceIndex shipDeviceIndex) : mShipDeviceIndex(shipDeviceIndex) {
}

ControllerMapping::~ControllerMapping() {
}

std::string ControllerMapping::GetPhysicalDeviceName() {
    return "Unknown";
}

ShipDeviceIndex ControllerMapping::GetShipDeviceIndex() {
    return mShipDeviceIndex;
}
} // namespace Ship
