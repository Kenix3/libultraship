#include "ship/controller/controldevice/controller/mapping/ControllerButtonMapping.h"

#include <random>
#include <sstream>

namespace Ship {
ControllerButtonMapping::ControllerButtonMapping(PhysicalDeviceType physicalDeviceType, uint8_t portIndex,
                                                 CONTROLLERBUTTONS_T bitmask)
    : ControllerInputMapping(physicalDeviceType), mPortIndex(portIndex), mBitmask(bitmask) {
}

ControllerButtonMapping::~ControllerButtonMapping() {
}

CONTROLLERBUTTONS_T ControllerButtonMapping::GetBitmask() {
    return mBitmask;
}

int8_t ControllerButtonMapping::GetMappingType() {
    return MAPPING_TYPE_UNKNOWN;
}

ImGuiKey ControllerButtonMapping::GetGamepadKey() {
    return ImGuiKey_None;
}

void ControllerButtonMapping::SetPortIndex(uint8_t portIndex) {
    mPortIndex = portIndex;
}
} // namespace Ship
