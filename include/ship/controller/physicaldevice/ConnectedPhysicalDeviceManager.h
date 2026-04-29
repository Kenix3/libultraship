#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <SDL2/SDL.h>

namespace Ship {

/**
 * @brief Tracks connected SDL game controllers and manages per-port ignore lists.
 *
 * ConnectedPhysicalDeviceManager maintains a live map of SDL game controllers that
 * are currently connected to the system. It also supports per-port ignore lists so
 * that specific controllers can be excluded from a given controller port's input
 * processing.
 */
class ConnectedPhysicalDeviceManager {
  public:
    /** @brief Constructs the manager and performs an initial scan of connected gamepads. */
    ConnectedPhysicalDeviceManager();
    ~ConnectedPhysicalDeviceManager();

    /**
     * @brief Returns the connected SDL gamepads available for the given port.
     *
     * Gamepads on the port's ignore list are excluded from the result.
     *
     * @param portIndex Zero-based controller port index.
     * @return Map of SDL joystick instance ID to SDL_GameController pointer.
     */
    std::unordered_map<int32_t, SDL_GameController*> GetConnectedSDLGamepadsForPort(uint8_t portIndex);

    /**
     * @brief Returns the display names of all connected SDL gamepads.
     * @return Map of SDL joystick instance ID to human-readable gamepad name.
     */
    std::unordered_map<int32_t, std::string> GetConnectedSDLGamepadNames();

    /**
     * @brief Returns the set of SDL joystick instance IDs ignored for a port.
     * @param portIndex Zero-based controller port index.
     * @return Set of ignored joystick instance IDs.
     */
    std::unordered_set<int32_t> GetIgnoredInstanceIdsForPort(uint8_t portIndex);

    /**
     * @brief Checks whether a specific joystick instance is being ignored on a port.
     * @param portIndex  Zero-based controller port index.
     * @param instanceId SDL joystick instance ID.
     * @return true if the instance is ignored for the given port.
     */
    bool PortIsIgnoringInstanceId(uint8_t portIndex, int32_t instanceId);

    /**
     * @brief Adds a joystick instance to a port's ignore list.
     * @param portIndex  Zero-based controller port index.
     * @param instanceId SDL joystick instance ID to ignore.
     */
    void IgnoreInstanceIdForPort(uint8_t portIndex, int32_t instanceId);

    /**
     * @brief Removes a joystick instance from a port's ignore list.
     * @param portIndex  Zero-based controller port index.
     * @param instanceId SDL joystick instance ID to stop ignoring.
     */
    void UnignoreInstanceIdForPort(uint8_t portIndex, int32_t instanceId);

    /**
     * @brief Handles an SDL device-added event by opening the new controller.
     * @param sdlDeviceIndex SDL device index from the SDL_CONTROLLERDEVICEADDED event.
     */
    void HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex);

    /**
     * @brief Handles an SDL device-removed event by closing the controller.
     * @param sdlJoystickInstanceId SDL joystick instance ID from the SDL_CONTROLLERDEVICEREMOVED event.
     */
    void HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId);

    /** @brief Re-scans all connected SDL gamepads and rebuilds the internal maps. */
    void RefreshConnectedSDLGamepads();

  private:
    std::unordered_map<int32_t, SDL_GameController*> mConnectedSDLGamepads;
    std::unordered_map<int32_t, std::string> mConnectedSDLGamepadNames;
    std::unordered_map<uint8_t, std::unordered_set<int32_t>> mIgnoredInstanceIds;
};
} // namespace Ship
