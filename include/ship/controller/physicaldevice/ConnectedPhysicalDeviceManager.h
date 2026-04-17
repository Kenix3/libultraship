#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <SDL2/SDL.h>

namespace Ship {

#ifdef ENABLE_PRESS_TO_JOIN
static constexpr int32_t KEYBOARD_PSEUDO_INSTANCE_ID = -2;
#endif

class ConnectedPhysicalDeviceManager {
  public:
    ConnectedPhysicalDeviceManager();
    ~ConnectedPhysicalDeviceManager();

    std::unordered_map<int32_t, SDL_GameController*> GetConnectedSDLGamepadsForPort(uint8_t portIndex);
    std::unordered_map<int32_t, std::string> GetConnectedSDLGamepadNames();
    std::unordered_set<int32_t> GetIgnoredInstanceIdsForPort(uint8_t portIndex);
    bool PortIsIgnoringInstanceId(uint8_t portIndex, int32_t instanceId);
    void IgnoreInstanceIdForPort(uint8_t portIndex, int32_t instanceId);
    void UnignoreInstanceIdForPort(uint8_t portIndex, int32_t instanceId);

    void HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex);
    void HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId);
    void RefreshConnectedSDLGamepads();

#ifdef ENABLE_PRESS_TO_JOIN
    void StartMultiplayer(uint8_t portCount);
    void StopPressToJoin();
    void StartPressToJoin();
    void StopMultiplayer();
    int8_t GetPortDeviceStatus(uint8_t port);
    void PollPressToJoin();
#endif

  private:
    std::unordered_map<int32_t, SDL_GameController*> mConnectedSDLGamepads;
    std::unordered_map<int32_t, std::string> mConnectedSDLGamepadNames;
    std::unordered_map<uint8_t, std::unordered_set<int32_t>> mIgnoredInstanceIds;

#ifdef ENABLE_PRESS_TO_JOIN
    bool mMultiplayerActive = false;
    bool mPressToJoinActive = false;
    uint8_t mMultiplayerPortCount = 1;
    std::unordered_map<uint8_t, int32_t> mPressToJoinAssignments;

    void AssignDeviceToPort(int32_t instanceId, uint8_t port);
    void UnassignPort(uint8_t port);
    void RebuildIgnoreListsFromAssignments();
    bool PortHasKeyboardMappings(uint8_t port);
#endif
};
} // namespace Ship
