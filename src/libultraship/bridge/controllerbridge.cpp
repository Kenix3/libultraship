#include "libultraship/bridge/controllerbridge.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/Context.h"

static std::shared_ptr<Ship::ControlDeck> sControlDeck;

static Ship::ControlDeck* GetControlDeck() {
    if (!sControlDeck) {
        sControlDeck = Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ControlDeck>();
    }
    return sControlDeck.get();
}


extern "C" {

void ControllerBlockGameInput(uint16_t inputBlockId) {
    GetControlDeck()->BlockGameInput(
        static_cast<int32_t>(inputBlockId));
}

void ControllerUnblockGameInput(uint16_t inputBlockId) {
    GetControlDeck()->UnblockGameInput(
        static_cast<int32_t>(inputBlockId));
}
}
