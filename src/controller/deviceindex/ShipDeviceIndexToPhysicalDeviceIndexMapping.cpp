#include "ShipDeviceIndexToPhysicalDeviceIndexMapping.h"

namespace Ship {
ShipDeviceIndexToPhysicalDeviceIndexMapping::ShipDeviceIndexToPhysicalDeviceIndexMapping(ShipDeviceType shipDeviceType)
    : mShipDeviceType(shipDeviceType) {
}

ShipDeviceIndexToPhysicalDeviceIndexMapping::~ShipDeviceIndexToPhysicalDeviceIndexMapping() {
}

ShipDeviceType ShipDeviceIndexToPhysicalDeviceIndexMapping::GetShipDeviceType() {
    return mShipDeviceType;
}

std::string ShipDeviceIndexToPhysicalDeviceIndexMapping::GetMappingId() {
#define X(name, value) \
    case name:         \
        return #name;
    switch (mShipDeviceType) {
        SHIP_DEVICE_TYPE_VALUES
        default:
            return "Max";
    }
#undef X
}
} // namespace Ship
