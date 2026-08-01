#include "ship/controller/physicaldevice/ConnectedPhysicalDeviceManager.h"
#include <spdlog/spdlog.h>

namespace Ship {
ConnectedPhysicalDeviceManager::ConnectedPhysicalDeviceManager() {
}

ConnectedPhysicalDeviceManager::~ConnectedPhysicalDeviceManager() {
    CloseConnectedSDLGamepads();
}

std::unordered_map<int32_t, SDL_Gamepad*>
ConnectedPhysicalDeviceManager::GetConnectedSDLGamepadsForPort(uint8_t portIndex) {
    std::unordered_map<int32_t, SDL_Gamepad*> result;

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

void ConnectedPhysicalDeviceManager::HandlePhysicalDeviceConnect(int32_t) {
    RefreshConnectedSDLGamepads();
}

void ConnectedPhysicalDeviceManager::HandlePhysicalDeviceDisconnect(int32_t) {
    RefreshConnectedSDLGamepads();
}

void ConnectedPhysicalDeviceManager::CloseConnectedSDLGamepads() {
    if ((SDL_WasInit(SDL_INIT_GAMEPAD) & SDL_INIT_GAMEPAD) != 0) {
        for (const auto& [instanceId, gamepad] : mConnectedSDLGamepads) {
            SDL_CloseGamepad(gamepad);
        }
    }
    mConnectedSDLGamepads.clear();
    mConnectedSDLGamepadNames.clear();
}

void ConnectedPhysicalDeviceManager::RefreshConnectedSDLGamepads() {
    static SDL_GUID sZeroGuid;

    int32_t joystickCount = 0;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&joystickCount);
    if (joysticks == nullptr) {
        return;
    }

    std::unordered_set<SDL_JoystickID> connectedInstanceIds(joysticks, joysticks + joystickCount);

    for (auto gamepadIt = mConnectedSDLGamepads.begin(); gamepadIt != mConnectedSDLGamepads.end();) {
        const SDL_JoystickID instanceId = gamepadIt->first;
        if (connectedInstanceIds.contains(instanceId)) {
            ++gamepadIt;
            continue;
        }

        SDL_CloseGamepad(gamepadIt->second);
        gamepadIt = mConnectedSDLGamepads.erase(gamepadIt);
        mConnectedSDLGamepadNames.erase(instanceId);
    }

    for (int32_t i = 0; i < joystickCount; i++) {
        const SDL_JoystickID instanceId = joysticks[i];

        if (mConnectedSDLGamepads.contains(instanceId)) {
            continue;
        }

        SDL_GUID deviceGUID = SDL_GetJoystickGUIDForID(instanceId);
        if (SDL_memcmp(&deviceGUID, &sZeroGuid, sizeof(deviceGUID)) == 0) {
            SPDLOG_WARN("SDL_GetJoystickGUIDForID returned a zero GUID for joystick instance {:d}; skipping it.",
                        instanceId);
            continue;
        }

        char deviceGuidCStr[33] = "";
        SDL_GUIDToString(deviceGUID, deviceGuidCStr, sizeof(deviceGuidCStr));

        if (!SDL_IsGamepad(instanceId)) {
            SPDLOG_WARN("SDL Joystick (GUID: {}) not recognized as gamepad."
                        "This is likely due to a missing mapping string in gamecontrollerdb.txt."
                        "Refer to https://github.com/mdqinc/SDL_GameControllerDB for more information.",
                        deviceGuidCStr);
            continue;
        }

        auto gamepad = SDL_OpenGamepad(instanceId);
        if (gamepad == nullptr) {
            SPDLOG_ERROR("SDL_OpenGamepad error (GUID: {}): {}", deviceGuidCStr, SDL_GetError());
            continue;
        }

        std::string gamepadName;
        auto name = SDL_GetGamepadName(gamepad);
        if (name == nullptr) {
            gamepadName = deviceGuidCStr;
            SPDLOG_WARN("SDL_GetGamepadName returned null. Setting name to GUID \"{}\" instead.", gamepadName);
        } else {
            gamepadName = name;
        }

        mConnectedSDLGamepads[instanceId] = gamepad;
        mConnectedSDLGamepadNames[instanceId] = gamepadName;

        for (uint8_t port = 1; port < 4; port++) {
            mIgnoredInstanceIds[port].insert(instanceId);
        }
    }

    SDL_free(joysticks);
}
} // namespace Ship
