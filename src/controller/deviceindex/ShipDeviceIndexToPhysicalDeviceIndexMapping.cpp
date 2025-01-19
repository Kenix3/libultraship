#include "ShipDeviceIndexToPhysicalDeviceIndexMapping.h"

namespace Ship {
ShipDeviceIndexToPhysicalDeviceIndexMapping::ShipDeviceIndexToPhysicalDeviceIndexMapping(
    PhysicalDeviceType physicalDeviceType)
    : mPhysicalDeviceType(physicalDeviceType) {
}

ShipDeviceIndexToPhysicalDeviceIndexMapping::~ShipDeviceIndexToPhysicalDeviceIndexMapping() {
}

PhysicalDeviceType ShipDeviceIndexToPhysicalDeviceIndexMapping::GetPhysicalDeviceType() {
    return mPhysicalDeviceType;
}

std::string ShipDeviceIndexToPhysicalDeviceIndexMapping::GetMappingId() {
#define X(name, value) \
    case name:         \
        return #name;
    switch (mPhysicalDeviceType) {
        PHYSICAL_DEVICE_TYPE_VALUES
        default:
            return "Max";
    }
#undef X
}
} // namespace Ship
