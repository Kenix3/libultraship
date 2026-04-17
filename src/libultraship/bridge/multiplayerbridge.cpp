#ifdef ENABLE_PRESS_TO_JOIN

#include "libultraship/bridge/multiplayerbridge.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/Context.h"

void MultiplayerStart(uint8_t portCount) {
    Ship::Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager()->StartMultiplayer(portCount);
}

void MultiplayerStopPressToJoin(void) {
    Ship::Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager()->StopPressToJoin();
}

void MultiplayerStartPressToJoin(void) {
    Ship::Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager()->StartPressToJoin();
}

void MultiplayerStop(void) {
    Ship::Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager()->StopMultiplayer();
}

int8_t MultiplayerGetPortStatus(uint8_t port) {
    return Ship::Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager()->GetPortDeviceStatus(port);
}

#endif // ENABLE_PRESS_TO_JOIN
