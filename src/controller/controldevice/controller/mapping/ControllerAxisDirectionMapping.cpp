#include "ControllerAxisDirectionMapping.h"

#include <random>
#include <sstream>

namespace ShipDK {
ControllerAxisDirectionMapping::ControllerAxisDirectionMapping(ShipDKDeviceIndex shipDKDeviceIndex, uint8_t portIndex,
                                                               Stick stick, Direction direction)
    : ControllerInputMapping(shipDKDeviceIndex), mPortIndex(portIndex), mStick(stick), mDirection(direction) {
}

ControllerAxisDirectionMapping::~ControllerAxisDirectionMapping() {
}

uint8_t ControllerAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_UNKNOWN;
}

Direction ControllerAxisDirectionMapping::GetDirection() {
    return mDirection;
}

void ControllerAxisDirectionMapping::SetPortIndex(uint8_t portIndex) {
    mPortIndex = portIndex;
}
} // namespace ShipDK
