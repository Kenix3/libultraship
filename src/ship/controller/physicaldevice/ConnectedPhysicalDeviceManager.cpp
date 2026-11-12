#include "ship/controller/physicaldevice/ConnectedPhysicalDeviceManager.h"
#include <algorithm>
#include <string_view>
#include <spdlog/spdlog.h>
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"

namespace {
constexpr uint8_t GamepadPortCount = 4;

std::string GetGamepadConfigId(SDL_GameController* gamepad, std::string id) {
#if SDL_VERSION_ATLEAST(2, 0, 14)
    const char* serial = SDL_GameControllerGetSerial(gamepad);
    if (serial != nullptr && serial[0] != '\0') {
        // Encode the serial so punctuation cannot be interpreted as a config path separator.
        constexpr char hex[] = "0123456789abcdef";
        id += '_';
        for (const unsigned char byte : std::string_view(serial)) {
            id += hex[byte >> 4];
            id += hex[byte & 0x0f];
        }
    }
#endif
    return id;
}

std::shared_ptr<Ship::ConsoleVariable> GetConsoleVariables() {
    const auto* context = Ship::Context::GetRawInstance();
    return context != nullptr ? context->GetConsoleVariables() : nullptr;
}

std::string GetPortConfigKey(uint8_t portIndex, const std::string& gamepadId) {
    return CVAR_PREFIX_CONTROLLERS ".Port" + std::to_string(portIndex + 1) + ".GamepadOverrides." + gamepadId;
}
} // namespace

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
    const auto port = mIgnoredInstanceIds.find(portIndex);
    return port != mIgnoredInstanceIds.end() && port->second.contains(instanceId);
}

void ConnectedPhysicalDeviceManager::IgnoreInstanceIdForPort(uint8_t portIndex, int32_t instanceId) {
    SetInstanceIdIgnoredForPort(portIndex, instanceId, true);
}

void ConnectedPhysicalDeviceManager::UnignoreInstanceIdForPort(uint8_t portIndex, int32_t instanceId) {
    SetInstanceIdIgnoredForPort(portIndex, instanceId, false);
}

const std::string* ConnectedPhysicalDeviceManager::GetUniqueGamepadConfigId(int32_t instanceId) const {
    const auto gamepad = mGamepadConfigIds.find(instanceId);
    if (gamepad == mGamepadConfigIds.end() || gamepad->second.empty()) {
        return nullptr;
    }

    // A GUID may identify only the model when SDL does not provide a serial number.
    const auto matches = std::count_if(mGamepadConfigIds.begin(), mGamepadConfigIds.end(),
                                       [&gamepad](const auto& other) { return other.second == gamepad->second; });
    return matches == 1 ? &gamepad->second : nullptr;
}

void ConnectedPhysicalDeviceManager::SetInstanceIdIgnoredForPort(uint8_t portIndex, int32_t instanceId, bool ignored) {
    if (ignored) {
        mIgnoredInstanceIds[portIndex].insert(instanceId);
    } else {
        mIgnoredInstanceIds[portIndex].erase(instanceId);
    }
    mSessionPortOverrides[instanceId][portIndex] = ignored;

    const auto* gamepadId = GetUniqueGamepadConfigId(instanceId);
    const auto consoleVariables = GetConsoleVariables();
    if (gamepadId != nullptr && consoleVariables != nullptr) {
        const auto key = GetPortConfigKey(portIndex, *gamepadId);
        if (ignored == (portIndex != 0)) {
            consoleVariables->ClearVariable(key.c_str());
        } else {
            consoleVariables->SetInteger(key.c_str(), ignored);
        }
        consoleVariables->Save();
    }
}

void ConnectedPhysicalDeviceManager::RestorePortSettings() {
    mIgnoredInstanceIds.clear();
    std::erase_if(mSessionPortOverrides,
                  [this](const auto& entry) { return !mConnectedSDLGamepads.contains(entry.first); });

    const auto consoleVariables = GetConsoleVariables();
    for (const auto& [instanceId, gamepad] : mConnectedSDLGamepads) {
        const auto* gamepadId = GetUniqueGamepadConfigId(instanceId);
        for (uint8_t portIndex = 0; portIndex < GamepadPortCount; portIndex++) {
            bool ignored = portIndex != 0;
            if (gamepadId != nullptr && consoleVariables != nullptr) {
                const auto key = GetPortConfigKey(portIndex, *gamepadId);
                ignored = consoleVariables->GetInteger(key.c_str(), ignored) != 0;
            }
            if (ignored) {
                mIgnoredInstanceIds[portIndex].insert(instanceId);
            }
        }
    }

    for (const auto& [instanceId, overrides] : mSessionPortOverrides) {
        for (const auto& [portIndex, ignored] : overrides) {
            if (ignored) {
                mIgnoredInstanceIds[portIndex].insert(instanceId);
            } else {
                mIgnoredInstanceIds[portIndex].erase(instanceId);
            }
        }
    }
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
    mGamepadConfigIds.clear();
    static SDL_JoystickGUID sZeroGuid;

    for (int32_t i = 0; i < SDL_NumJoysticks(); i++) {

        SDL_JoystickGUID deviceGUID = SDL_JoystickGetDeviceGUID(i);
        if (SDL_memcmp(&deviceGUID, &sZeroGuid, sizeof(deviceGUID)) == 0) {
            SPDLOG_WARN(
                "Calling SDL JoystickGetDeviceGUID with index ({:d}) returned zero GUID. This is likely due to an "
                "invalid index. Refer to https://wiki.libsdl.org/SDL2/SDL_JoystickGetDeviceGUID for more information.",
                i);
            continue;
        }

        char deviceGuidCStr[33] = "";
        SDL_JoystickGetGUIDString(deviceGUID, deviceGuidCStr, sizeof(deviceGuidCStr));

        if (!SDL_IsGameController(i)) {
            SPDLOG_WARN("SDL Joystick (GUID: {}) not recognized as gamepad."
                        "This is likely due to a missing mapping string in gamecontrollerdb.txt."
                        "Refer to https://github.com/mdqinc/SDL_GameControllerDB for more information.",
                        deviceGuidCStr);
            continue;
        }

        auto gamepad = SDL_GameControllerOpen(i);
        if (gamepad == nullptr) {
            SPDLOG_ERROR("SDL GameControllerOpen error (GUID: {}): {}", deviceGuidCStr, SDL_GetError());
            continue;
        }

        auto instanceId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad));
        if (instanceId < 0) {
            SPDLOG_ERROR("SDL JoystickInstanceID error (GUID: {}): {}", deviceGuidCStr, SDL_GetError());
            continue;
        }

        std::string gamepadName;
        auto name = SDL_GameControllerName(gamepad);
        if (name == nullptr) {
            gamepadName = deviceGuidCStr;
            SPDLOG_WARN("SDL_GameControllerName returned null. Setting name to GUID \"{}\" instead.", gamepadName);
        } else {
            gamepadName = name;
        }

        mConnectedSDLGamepads[instanceId] = gamepad;
        mConnectedSDLGamepadNames[instanceId] = gamepadName;
        mGamepadConfigIds[instanceId] = GetGamepadConfigId(gamepad, deviceGuidCStr);
    }
    RestorePortSettings();
}
} // namespace Ship
