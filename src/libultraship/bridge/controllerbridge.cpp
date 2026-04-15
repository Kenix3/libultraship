#include "libultraship/bridge/controllerbridge.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/Context.h"

extern "C" {

void ControllerBlockGameInput(uint16_t inputBlockId) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ControlDeck>()->BlockGameInput(static_cast<int32_t>(inputBlockId));
}

void ControllerUnblockGameInput(uint16_t inputBlockId) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ControlDeck>()->UnblockGameInput(static_cast<int32_t>(inputBlockId));
}
}
