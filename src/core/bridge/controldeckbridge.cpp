#include "core/bridge/controldeckbridge.h"
#include "core/Context.h"

extern "C" {

void BlockGameInput(uint16_t inputBlockId) {
    LUS::Context::GetInstance()->GetControlDeck()->BlockGameInput(static_cast<int32_t>(inputBlockId));
}

void UnblockGameInput(uint16_t inputBlockId) {
    LUS::Context::GetInstance()->GetControlDeck()->UnblockGameInput(static_cast<int32_t>(inputBlockId));
}
}
