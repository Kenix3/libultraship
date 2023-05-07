#include "core/bridge/controldeckbridge.h"
#include "core/Context.h"

extern "C" {

void BlockGameInput(void) {
    Ship::Context::GetInstance()->GetControlDeck()->BlockGameInput();
}

void UnblockGameInput(void) {
    Ship::Context::GetInstance()->GetControlDeck()->UnblockGameInput();
}
}
