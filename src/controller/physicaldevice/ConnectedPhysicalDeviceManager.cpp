#include "ConnectedPhysicalDeviceManager.h"

namespace Ship {
ConnectedPhysicalDeviceManager::ConnectedPhysicalDeviceManager() {
}

ConnectedPhysicalDeviceManager::~ConnectedPhysicalDeviceManager() {
}

std::unordered_map<int32_t, SDL_GameController*>
ConnectedPhysicalDeviceManager::GetConnectedSDLGamepadsForPort(uint8_t portIndex) {
    // todo: filter somehow
    return mConnectedSDLGamepads;
}

std::vector<std::string> ConnectedPhysicalDeviceManager::GetConnectedSDLGamepadNames() {
    return mConnectedSDLGamepadNames;
}

void ConnectedPhysicalDeviceManager::HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex) {
    RefreshConnectedSDLGamepads();
}

void ConnectedPhysicalDeviceManager::HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId) {
    RefreshConnectedSDLGamepads();
}

void ConnectedPhysicalDeviceManager::RefreshConnectedSDLGamepads() {
    mConnectedSDLGamepads.clear();
    mConnectedSDLGamepadNames.clear();
    for (int32_t i = 0; i < SDL_NumJoysticks(); i++) {
        // skip if this SDL joystick isn't a Gamepad
        if (!SDL_IsGameController(i)) {
            continue;
        }

        auto gamepad = SDL_GameControllerOpen(i);
        auto instanceId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad));
        auto name = SDL_GameControllerName(gamepad);

        mConnectedSDLGamepads[instanceId] = gamepad;
        mConnectedSDLGamepadNames.push_back(name);
    }
}
} // namespace Ship
