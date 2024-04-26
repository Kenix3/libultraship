#include "ControllerButtonMapping.h"

#include <random>
#include <sstream>

#include "public/bridge/consolevariablebridge.h"

namespace ShipDK {
ControllerButtonMapping::ControllerButtonMapping(ShipDKDeviceIndex shipDKDeviceIndex, uint8_t portIndex,
                                                 CONTROLLERBUTTONS_T bitmask)
    : ControllerInputMapping(shipDKDeviceIndex), mPortIndex(portIndex), mBitmask(bitmask) {
}

ControllerButtonMapping::~ControllerButtonMapping() {
}

CONTROLLERBUTTONS_T ControllerButtonMapping::GetBitmask() {
    return mBitmask;
}

uint8_t ControllerButtonMapping::GetMappingType() {
    return MAPPING_TYPE_UNKNOWN;
}

void ControllerButtonMapping::SetPortIndex(uint8_t portIndex) {
    mPortIndex = portIndex;
}
} // namespace ShipDK
