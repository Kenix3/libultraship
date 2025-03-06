#include "ControllerAxisDirectionMapping.h"

#include <random>
#include <sstream>

namespace Ship {
ControllerAxisDirectionMapping::ControllerAxisDirectionMapping(PhysicalDeviceType physicalDeviceType, uint8_t portIndex,
                                                               StickIndex stickIndex, Direction direction)
    : ControllerInputMapping(physicalDeviceType), mPortIndex(portIndex), mStickIndex(stickIndex),
      mDirection(direction) {
}

ControllerAxisDirectionMapping::~ControllerAxisDirectionMapping() {
}

int8_t ControllerAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_UNKNOWN;
}

Direction ControllerAxisDirectionMapping::GetDirection() {
    return mDirection;
}

void ControllerAxisDirectionMapping::SetPortIndex(uint8_t portIndex) {
    mPortIndex = portIndex;
}
} // namespace Ship
