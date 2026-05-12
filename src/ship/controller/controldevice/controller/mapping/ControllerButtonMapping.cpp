#include "ship/controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/config/Config.h"

#include <random>
#include <sstream>
#include <stdexcept>

namespace Ship {
ControllerButtonMapping::ControllerButtonMapping(PhysicalDeviceType physicalDeviceType, uint8_t portIndex,
                                                 CONTROLLERBUTTONS_T bitmask, std::shared_ptr<ControlDeck> controlDeck,
                                                 std::shared_ptr<Config> config)
    : ControllerInputMapping(physicalDeviceType), mPortIndex(portIndex), mBitmask(bitmask),
      mControlDeck(std::move(controlDeck)), mConfig(std::move(config)) {
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

std::shared_ptr<Config> ControllerButtonMapping::GetConfig() const {
    if (!mConfig) {
        throw std::runtime_error("ControllerButtonMapping requires Config dependency");
    }
    if (!mConfig->IsInitialized()) {
        throw std::runtime_error("ControllerButtonMapping requires Config to be initialized");
    }
    return mConfig;
}
} // namespace Ship
