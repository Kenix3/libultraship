#pragma once

#ifdef __WIIU__
#include "LUSDeviceIndexToWiiUDeviceIndexMapping.h"
#else
#include "LUSDeviceIndexToSDLDeviceIndexMapping.h"
#endif

#include <unordered_map>
#include <memory>
#include <vector>

namespace LUS {
class LUSDeviceIndexMappingManager {
  public:
    LUSDeviceIndexMappingManager();
    ~LUSDeviceIndexMappingManager();

#ifdef __WIIU__
    void InitializeMappingsMultiplayer(std::vector<int32_t> wiiuDeviceChannels);
    void InitializeWiiUMappingsForPort(uint8_t n64port, bool isGamepad, int32_t wiiuChannel);
    bool IsValidWiiUExtensionType(int32_t extensionType);
    void UpdateExtensionTypesFromConfig();
    std::pair<bool, int32_t> GetWiiUDeviceTypeFromLUSDeviceIndex(LUSDeviceIndex index);
    void HandlePhysicalDevicesChanged();
#else
    void InitializeMappingsMultiplayer(std::vector<int32_t> sdlIndices);
    void InitializeSDLMappingsForPort(uint8_t n64port, int32_t sdlIndex);
    void UpdateControllerNamesFromConfig();
    std::string GetSDLControllerNameFromLUSDeviceIndex(LUSDeviceIndex index);
    void HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex);
    void HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId);
    LUSDeviceIndex GetLUSDeviceIndexFromSDLDeviceIndex(int32_t sdlIndex);
#endif

    std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping> CreateDeviceIndexMappingFromConfig(std::string id);

    std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>>
    GetAllDeviceIndexMappings();
    std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>>
    GetAllDeviceIndexMappingsFromConfig();
    std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>
    GetDeviceIndexMappingFromLUSDeviceIndex(LUSDeviceIndex lusIndex);
    void
    SetLUSDeviceIndexToPhysicalDeviceIndexMapping(std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping> mapping);
    void RemoveLUSDeviceIndexToPhysicalDeviceIndexMapping(LUSDeviceIndex index);
    LUSDeviceIndex GetLowestLUSDeviceIndexWithNoAssociatedButtonOrAxisDirectionMappings(); // did this name for the meme

    void InitializeMappingsSinglePlayer();

    void SaveMappingIdsToConfig();

  private:
    bool mIsInitialized;
    std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>>
        mLUSDeviceIndexToPhysicalDeviceIndexMappings;
#ifdef __WIIU__
    std::unordered_map<LUSDeviceIndex, std::pair<bool, int32_t>> mLUSDeviceIndexToWiiUDeviceTypes;
#else
    std::unordered_map<LUSDeviceIndex, std::string> mLUSDeviceIndexToSDLControllerNames;
    int32_t GetNewSDLDeviceIndexFromLUSDeviceIndex(LUSDeviceIndex lusIndex);
    LUSDeviceIndex GetLUSDeviceIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId);
    uint8_t GetPortIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId);
    void HandlePhysicalDeviceDisconnectSinglePlayer(int32_t sdlJoystickInstanceId);
    void HandlePhysicalDeviceDisconnectMultiplayer(int32_t sdlJoystickInstanceId);
#endif
};
} // namespace LUS
