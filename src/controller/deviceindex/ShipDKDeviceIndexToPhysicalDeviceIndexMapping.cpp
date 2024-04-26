#include "ShipDKDeviceIndexToPhysicalDeviceIndexMapping.h"

namespace ShipDK {
ShipDKDeviceIndexToPhysicalDeviceIndexMapping::ShipDKDeviceIndexToPhysicalDeviceIndexMapping(ShipDKDeviceIndex shipDKDeviceIndex)
    : mShipDKDeviceIndex(shipDKDeviceIndex) {
}

ShipDKDeviceIndexToPhysicalDeviceIndexMapping::~ShipDKDeviceIndexToPhysicalDeviceIndexMapping() {
}

ShipDKDeviceIndex ShipDKDeviceIndexToPhysicalDeviceIndexMapping::GetShipDKDeviceIndex() {
    return mShipDKDeviceIndex;
}

std::string ShipDKDeviceIndexToPhysicalDeviceIndexMapping::GetMappingId() {
#define X(name, value) \
    case name:         \
        return #name;
    switch (mShipDKDeviceIndex) {
        SHIPDK_DEVICE_INDEX_VALUES
        default:
            return "Max";
    }
#undef X
}
} // namespace ShipDK
