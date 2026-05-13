#include "ship/controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "ship/controller/controldeck/ControlDeck.h"

#include <random>
#include <sstream>
#include <stdexcept>

namespace Ship {
ControllerAxisDirectionMapping::ControllerAxisDirectionMapping(PhysicalDeviceType physicalDeviceType, uint8_t portIndex,
                                                               StickIndex stickIndex, Direction direction,
                                                               std::shared_ptr<ControlDeck> controlDeck)
    : ControllerInputMapping(physicalDeviceType), mPortIndex(portIndex), mStickIndex(stickIndex),
      mDirection(direction), mControlDeck(std::move(controlDeck)) {
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

std::shared_ptr<ControlDeck> ControllerAxisDirectionMapping::GetControlDeck() const {
    if (!mControlDeck) {
        throw std::runtime_error("ControllerAxisDirectionMapping requires ControlDeck dependency");
    }
    if (!mControlDeck->IsInitialized()) {
        throw std::runtime_error("ControllerAxisDirectionMapping requires ControlDeck to be initialized");
    }
    return mControlDeck;
}
} // namespace Ship
