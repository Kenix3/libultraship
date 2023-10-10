#include "ControllerButtonMapping.h"

#include <random>
#include <sstream>

#include "public/bridge/consolevariablebridge.h"

namespace LUS {
ControllerButtonMapping::ControllerButtonMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, uint16_t bitmask)
    : ControllerInputMapping(lusDeviceIndex), mPortIndex(portIndex), mBitmask(bitmask) {
}

ControllerButtonMapping::~ControllerButtonMapping() {
}

uint16_t ControllerButtonMapping::GetBitmask() {
    return mBitmask;
}

uint8_t ControllerButtonMapping::GetMappingType() {
    return MAPPING_TYPE_UNKNOWN;
}

void ControllerButtonMapping::SetPortIndex(uint8_t portIndex) {
    mPortIndex = portIndex;
}
} // namespace LUS
