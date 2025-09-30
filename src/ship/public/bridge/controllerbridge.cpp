#include "public/bridge/controllerbridge.h"
#include "controller/controldeck/ControlDeck.h"
#include "Context.h"

extern "C" {

void ControllerBlockGameInput(uint16_t inputBlockId) {
    Ship::Context::GetInstance()->GetControlDeck()->BlockGameInput(static_cast<int32_t>(inputBlockId));
}

void ControllerUnblockGameInput(uint16_t inputBlockId) {
    Ship::Context::GetInstance()->GetControlDeck()->UnblockGameInput(static_cast<int32_t>(inputBlockId));
}
}
