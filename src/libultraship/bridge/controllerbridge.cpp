#include "libultraship/bridge/controllerbridge.h"
#include "ship/controller/controldeck/ControlDeck.h"

static std::shared_ptr<Ship::ControlDeck> sControlDeck;

void ControllerSetControlDeck(std::shared_ptr<Ship::ControlDeck> controlDeck) {
    sControlDeck = std::move(controlDeck);
}

std::shared_ptr<Ship::ControlDeck> ControllerGetControlDeck() {
    return sControlDeck;
}

static Ship::ControlDeck* GetControlDeck() {
    return sControlDeck.get();
}

extern "C" {

void ControllerBlockGameInput(uint16_t inputBlockId) {
    GetControlDeck()->BlockGameInput(static_cast<int32_t>(inputBlockId));
}

void ControllerUnblockGameInput(uint16_t inputBlockId) {
    GetControlDeck()->UnblockGameInput(static_cast<int32_t>(inputBlockId));
}
}
