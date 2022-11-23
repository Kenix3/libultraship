#include "core/bridge/controldeckbridge.h"
#include "core/Window.h"

extern "C" {

void BlockGameInput(void) {
    Ship::Window::GetInstance()->GetControlDeck()->BlockGameInput();
}

void UnblockGameInput(void) {
    Ship::Window::GetInstance()->GetControlDeck()->UnblockGameInput();
}
}
