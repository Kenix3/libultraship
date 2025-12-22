#include "ship/controller/physicaldevice/ConnectedPhysicalDeviceManager.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include <cstdint>

namespace Ship {
ConnectedPhysicalDeviceManager::ConnectedPhysicalDeviceManager() {
}

ConnectedPhysicalDeviceManager::~ConnectedPhysicalDeviceManager() {
}

std::unordered_map<int32_t, SDL_GameController*>
ConnectedPhysicalDeviceManager::GetConnectedSDLGamepadsForPort(uint8_t portIndex) {
    std::unordered_map<int32_t, SDL_GameController*> result;

    for (const auto& [instanceId, gamepad] : mConnectedSDLGamepads) {
        if (!PortIsIgnoringInstanceId(portIndex, instanceId)) {
            result[instanceId] = gamepad;
        }
    }

    return result;
}

std::unordered_map<int32_t, std::string> ConnectedPhysicalDeviceManager::GetConnectedSDLGamepadNames() {
    return mConnectedSDLGamepadNames;
}

std::unordered_set<int32_t> ConnectedPhysicalDeviceManager::GetIgnoredInstanceIdsForPort(uint8_t portIndex) {
    return mIgnoredInstanceIds[portIndex];
}

bool ConnectedPhysicalDeviceManager::PortIsIgnoringInstanceId(uint8_t portIndex, int32_t instanceId) {
    return GetIgnoredInstanceIdsForPort(portIndex).contains(instanceId);
}

void ConnectedPhysicalDeviceManager::IgnoreInstanceIdForPort(uint8_t portIndex, int32_t instanceId) {
    mIgnoredInstanceIds[portIndex].insert(instanceId);
}

void ConnectedPhysicalDeviceManager::UnignoreInstanceIdForPort(uint8_t portIndex, int32_t instanceId) {
    mIgnoredInstanceIds[portIndex].erase(instanceId);
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

#ifdef ENABLE_EXP_AUTO_MULTIPLAYER_CONTROLLERS
    for (uint8_t i = 0; i < 4; i++) {
        mIgnoredInstanceIds[i].clear();
    }

    uint8_t port = 0;
#endif

    for (int32_t i = 0; i < SDL_NumJoysticks(); i++) {
        // skip if this SDL joystick isn't a Gamepad
        if (!SDL_IsGameController(i)) {
            continue;
        }

        auto gamepad = SDL_GameControllerOpen(i);
        auto instanceId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad));
        auto name = SDL_GameControllerName(gamepad);

        mConnectedSDLGamepads[instanceId] = gamepad;
        mConnectedSDLGamepadNames[instanceId] = name;

#ifdef ENABLE_EXP_AUTO_MULTIPLAYER_CONTROLLERS
        bool autoConfigureEnabled =
            Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_AUTO_MULTIPLAYER_CONTROLLERS, 0);

        // Only auto-assign controllers to specific ports if enabled
        if (autoConfigureEnabled) {
            for (uint8_t j = 0; j < 4; j++) {
                if (port == j) {
                    continue;
                }
                mIgnoredInstanceIds[j].insert(instanceId);
            }
            port++;
        } else {
            for (uint8_t p = 1; p < 4; p++) {
                mIgnoredInstanceIds[p].insert(instanceId);
            }
        }
#else
        for (uint8_t port = 1; port < 4; port++) {
            mIgnoredInstanceIds[port].insert(instanceId);
        }
#endif
    }
}
} // namespace Ship
