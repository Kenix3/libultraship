#include "ship/controller/physicaldevice/ConnectedPhysicalDeviceManager.h"
#include <spdlog/spdlog.h>
#ifdef ENABLE_PRESS_TO_JOIN
#include "ship/Context.h"
#include "ship/window/Window.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/controller/controldevice/controller/mapping/ControllerMapping.h"
#include "libultraship/bridge/consolevariablebridge.h"
#endif

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
#ifdef ENABLE_PRESS_TO_JOIN
    // If press-to-join is active, free any port that had this device so it can be re-joined
    if (mPressToJoinActive) {
        for (auto& [port, assignedId] : mPressToJoinAssignments) {
            if (assignedId == sdlJoystickInstanceId) {
                UnassignPort(port);
                break;
            }
        }
    }
#endif
    RefreshConnectedSDLGamepads();
}

void ConnectedPhysicalDeviceManager::RefreshConnectedSDLGamepads() {
    mConnectedSDLGamepads.clear();
    mConnectedSDLGamepadNames.clear();
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

#ifdef ENABLE_PRESS_TO_JOIN
        if (!mMultiplayerActive) {
#endif
            for (uint8_t port = 1; port < 4; port++) {
                mIgnoredInstanceIds[port].insert(instanceId);
            }
#ifdef ENABLE_PRESS_TO_JOIN
        }
#endif
    }

#ifdef ENABLE_PRESS_TO_JOIN
    if (mMultiplayerActive) {
        RebuildIgnoreListsFromAssignments();
    }
#endif
}
#ifdef ENABLE_PRESS_TO_JOIN
static const char* CVAR_PRESS_TO_JOIN_ENABLED = "gPressToJoinEnabled";

void ConnectedPhysicalDeviceManager::StartMultiplayer(uint8_t portCount) {
    if (!CVarGetInteger(CVAR_PRESS_TO_JOIN_ENABLED, 1)) {
        return;
    }

    mMultiplayerActive = true;
    mPressToJoinActive = true;
    mMultiplayerPortCount = portCount;
    mPressToJoinAssignments.clear();

    // Ignore all devices on all active ports — everyone starts unassigned
    for (uint8_t port = 0; port < mMultiplayerPortCount; port++) {
        for (const auto& [instanceId, gamepad] : mConnectedSDLGamepads) {
            mIgnoredInstanceIds[port].insert(instanceId);
        }
    }

    SPDLOG_INFO("Press-to-join: started multiplayer with {} ports", portCount);
}

void ConnectedPhysicalDeviceManager::StopPressToJoin() {
    if (!mMultiplayerActive) {
        return;
    }

    mPressToJoinActive = false;
    SPDLOG_INFO("Press-to-join: stopped (assignments locked)");
}

void ConnectedPhysicalDeviceManager::StartPressToJoin() {
    if (!mMultiplayerActive) {
        return;
    }

    if (!CVarGetInteger(CVAR_PRESS_TO_JOIN_ENABLED, 1)) {
        return;
    }

    mPressToJoinActive = true;

    // Free any ports whose devices have disconnected
    std::vector<uint8_t> portsToFree;
    for (const auto& [port, instanceId] : mPressToJoinAssignments) {
        if (instanceId == KEYBOARD_PSEUDO_INSTANCE_ID) {
            continue;
        }
        if (!mConnectedSDLGamepads.contains(instanceId)) {
            portsToFree.push_back(port);
        }
    }
    for (auto port : portsToFree) {
        UnassignPort(port);
    }

    SPDLOG_INFO("Press-to-join: re-enabled");
}

void ConnectedPhysicalDeviceManager::StopMultiplayer() {
    mMultiplayerActive = false;
    mPressToJoinActive = false;
    mMultiplayerPortCount = 1;
    mPressToJoinAssignments.clear();

    // Restore default all-on-port-0 behavior
    RefreshConnectedSDLGamepads();

    SPDLOG_INFO("Press-to-join: stopped multiplayer, restored single player mode");
}

int8_t ConnectedPhysicalDeviceManager::GetPortDeviceStatus(uint8_t port) {
    if (!mPressToJoinAssignments.contains(port)) {
        return 0; // unassigned
    }

    auto instanceId = mPressToJoinAssignments[port];
    if (instanceId == KEYBOARD_PSEUDO_INSTANCE_ID) {
        return 1; // keyboard is always "connected"
    }

    if (mConnectedSDLGamepads.contains(instanceId)) {
        return 1; // assigned and connected
    }

    return -1; // assigned but disconnected
}

void ConnectedPhysicalDeviceManager::PollPressToJoin() {
    if (!mPressToJoinActive) {
        return;
    }

    static constexpr int32_t AXIS_DEADZONE = 16000;

    for (uint8_t port = 0; port < mMultiplayerPortCount; port++) {
        if (mPressToJoinAssignments.contains(port)) {
            continue; // port already filled
        }

        // Check keyboard for this port
        if (PortHasKeyboardMappings(port)) {
            auto lastScancode = Context::GetInstance()->GetWindow()->GetLastScancode();
            if (lastScancode != -1) {
                AssignDeviceToPort(KEYBOARD_PSEUDO_INSTANCE_ID, port);
                SPDLOG_INFO("Press-to-join: keyboard assigned to port {}", port);
                continue;
            }
        }

        // Check unassigned SDL gamepads
        for (const auto& [instanceId, gamepad] : mConnectedSDLGamepads) {
            // Skip if already assigned to any port
            bool alreadyAssigned = false;
            for (const auto& [assignedPort, assignedId] : mPressToJoinAssignments) {
                if (assignedId == instanceId) {
                    alreadyAssigned = true;
                    break;
                }
            }
            if (alreadyAssigned) {
                continue;
            }

            // Check any button pressed
            bool inputDetected = false;
            for (int btn = 0; btn < SDL_CONTROLLER_BUTTON_MAX; btn++) {
                if (SDL_GameControllerGetButton(gamepad, static_cast<SDL_GameControllerButton>(btn))) {
                    inputDetected = true;
                    break;
                }
            }

            // Check any axis past deadzone
            if (!inputDetected) {
                for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; axis++) {
                    auto value = SDL_GameControllerGetAxis(gamepad, static_cast<SDL_GameControllerAxis>(axis));
                    if (abs(value) > AXIS_DEADZONE) {
                        inputDetected = true;
                        break;
                    }
                }
            }

            if (inputDetected) {
                AssignDeviceToPort(instanceId, port);
                SPDLOG_INFO("Press-to-join: gamepad {} assigned to port {}", instanceId, port);
                break; // one assignment per frame per port
            }
        }
    }
}

void ConnectedPhysicalDeviceManager::AssignDeviceToPort(int32_t instanceId, uint8_t port) {
    mPressToJoinAssignments[port] = instanceId;

    if (instanceId != KEYBOARD_PSEUDO_INSTANCE_ID) {
        // Unignore this device on the target port
        UnignoreInstanceIdForPort(port, instanceId);

        // Ignore it on all other active ports
        for (uint8_t otherPort = 0; otherPort < mMultiplayerPortCount; otherPort++) {
            if (otherPort != port) {
                IgnoreInstanceIdForPort(otherPort, instanceId);
            }
        }
    }
}

void ConnectedPhysicalDeviceManager::UnassignPort(uint8_t port) {
    if (!mPressToJoinAssignments.contains(port)) {
        return;
    }

    auto instanceId = mPressToJoinAssignments[port];
    mPressToJoinAssignments.erase(port);

    if (instanceId != KEYBOARD_PSEUDO_INSTANCE_ID) {
        // Re-ignore this device on the port it was assigned to
        IgnoreInstanceIdForPort(port, instanceId);
    }

    SPDLOG_INFO("Press-to-join: port {} unassigned", port);
}

void ConnectedPhysicalDeviceManager::RebuildIgnoreListsFromAssignments() {
    // Clear ignore lists for all active ports
    for (uint8_t port = 0; port < mMultiplayerPortCount; port++) {
        mIgnoredInstanceIds[port].clear();
    }

    // For each active port, ignore all devices except the one assigned to it
    for (uint8_t port = 0; port < mMultiplayerPortCount; port++) {
        for (const auto& [instanceId, gamepad] : mConnectedSDLGamepads) {
            if (mPressToJoinAssignments.contains(port) &&
                mPressToJoinAssignments[port] == instanceId) {
                continue; // don't ignore the assigned device
            }
            mIgnoredInstanceIds[port].insert(instanceId);
        }
    }
}

bool ConnectedPhysicalDeviceManager::PortHasKeyboardMappings(uint8_t port) {
    auto controller = Context::GetInstance()->GetControlDeck()->GetControllerByPort(port);
    if (controller == nullptr) {
        return false;
    }

    for (const auto& [bitmask, button] : controller->GetAllButtons()) {
        for (const auto& [id, mapping] : button->GetAllButtonMappings()) {
            if (mapping->GetMappingType() == MAPPING_TYPE_KEYBOARD) {
                return true;
            }
        }
    }

    return false;
}
#endif
} // namespace Ship
