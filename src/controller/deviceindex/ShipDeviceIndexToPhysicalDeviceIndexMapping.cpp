#include "ShipDeviceIndexToPhysicalDeviceIndexMapping.h"

namespace Ship {
ShipDeviceIndexToPhysicalDeviceIndexMapping::ShipDeviceIndexToPhysicalDeviceIndexMapping(
    ShipDeviceIndex shipDeviceIndex)
    : mShipDeviceIndex(shipDeviceIndex) {
}

ShipDeviceIndexToPhysicalDeviceIndexMapping::~ShipDeviceIndexToPhysicalDeviceIndexMapping() {
}

ShipDeviceIndex ShipDeviceIndexToPhysicalDeviceIndexMapping::GetShipDeviceIndex() {
    return mShipDeviceIndex;
}

std::string ShipDeviceIndexToPhysicalDeviceIndexMapping::GetMappingId() {
#define X(name, value) \
    case name:         \
        return #name;
    switch (mShipDeviceIndex) {
        SHIPDK_DEVICE_INDEX_VALUES
        default:
            return "Max";
    }
#undef X
}
} // namespace Ship
