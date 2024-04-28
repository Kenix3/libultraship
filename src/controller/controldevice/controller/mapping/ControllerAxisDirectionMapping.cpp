#include "ControllerAxisDirectionMapping.h"

#include <random>
#include <sstream>

namespace Ship {
ControllerAxisDirectionMapping::ControllerAxisDirectionMapping(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex,
                                                               Stick stick, Direction direction)
    : ControllerInputMapping(shipDeviceIndex), mPortIndex(portIndex), mStick(stick), mDirection(direction) {
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
} // namespace Ship
