#include "core/bridge/controldeckbridge.h"
#include "core/Context.h"

extern "C" {

void BlockGameInput(void) {
    LUS::Context::GetInstance()->GetControlDeck()->BlockGameInput();
}

void UnblockGameInput(void) {
    LUS::Context::GetInstance()->GetControlDeck()->UnblockGameInput();
}
}
