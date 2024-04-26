#include "ControllerMapping.h"

namespace ShipDK {
ControllerMapping::ControllerMapping(ShipDKDeviceIndex shipDKDeviceIndex) : mShipDKDeviceIndex(shipDKDeviceIndex) {
}

ControllerMapping::~ControllerMapping() {
}

std::string ControllerMapping::GetPhysicalDeviceName() {
    return "Unknown";
}

ShipDKDeviceIndex ControllerMapping::GetShipDKDeviceIndex() {
    return mShipDKDeviceIndex;
}
} // namespace ShipDK
