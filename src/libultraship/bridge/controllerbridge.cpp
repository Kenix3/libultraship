#include "libultraship/bridge/controllerbridge.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include <atomic>

static std::atomic<std::shared_ptr<Ship::ControlDeck>> sControlDeck;

void ControllerSetControlDeck(std::shared_ptr<Ship::ControlDeck> controlDeck) {
    sControlDeck.store(std::move(controlDeck), std::memory_order_release);
}

std::shared_ptr<Ship::ControlDeck> ControllerGetControlDeck() {
    return sControlDeck.load(std::memory_order_acquire);
}

extern "C" {

void ControllerBlockGameInput(uint16_t inputBlockId) {
    if (auto controlDeck = ControllerGetControlDeck()) {
        controlDeck->BlockGameInput(static_cast<int32_t>(inputBlockId));
    }
}

void ControllerUnblockGameInput(uint16_t inputBlockId) {
    if (auto controlDeck = ControllerGetControlDeck()) {
        controlDeck->UnblockGameInput(static_cast<int32_t>(inputBlockId));
    }
}
}
