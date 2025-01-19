#include "ConnectedPhysicalDeviceManager.h"

namespace Ship {
ConnectedPhysicalDeviceManager::ConnectedPhysicalDeviceManager() {
}

ConnectedPhysicalDeviceManager::~ConnectedPhysicalDeviceManager() {
}

std::vector<SDL_GameController*> ConnectedPhysicalDeviceManager::GetConnectedSDLGamepadsForPort(uint8_t portIndex) {
    // todo: filter somehow
    return mConnectedSDLGamepads;
}
} // namespace Ship
