#include "ship/controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "ship/controller/controldeck/ControlDeck.h"

#include <random>
#include <sstream>
#include <stdexcept>

namespace Ship {
ControllerButtonMapping::ControllerButtonMapping(PhysicalDeviceType physicalDeviceType, uint8_t portIndex,
                                                 CONTROLLERBUTTONS_T bitmask, std::shared_ptr<ControlDeck> controlDeck)
    : ControllerInputMapping(physicalDeviceType), mPortIndex(portIndex), mBitmask(bitmask),
      mControlDeck(std::move(controlDeck)) {
}

ControllerButtonMapping::~ControllerButtonMapping() {
}

CONTROLLERBUTTONS_T ControllerButtonMapping::GetBitmask() {
    return mBitmask;
}

int8_t ControllerButtonMapping::GetMappingType() {
    return MAPPING_TYPE_UNKNOWN;
}

void ControllerButtonMapping::SetPortIndex(uint8_t portIndex) {
    mPortIndex = portIndex;
}

std::shared_ptr<ControlDeck> ControllerButtonMapping::GetControlDeck() const {
    if (!mControlDeck) {
        throw std::runtime_error("ControllerButtonMapping requires ControlDeck dependency");
    }
    if (!mControlDeck->IsInitialized()) {
        throw std::runtime_error("ControllerButtonMapping requires ControlDeck to be initialized");
    }
    return mControlDeck;
}
} // namespace Ship