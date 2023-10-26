#pragma once

#include "LUSDeviceIndexToSDLDeviceIndexMapping.h"
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
    void InitializeWiiUMappingsForPort(uint8_t n64port, int32_t wiiuChannel);
    #else
    void InitializeMappingsMultiplayer(std::vector<int32_t> sdlIndices);
    void InitializeSDLMappingsForPort(uint8_t n64port, int32_t sdlIndex);
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
    LUSDeviceIndex GetLUSDeviceIndexFromSDLDeviceIndex(int32_t sdlIndex);
    std::string GetSDLControllerNameFromLUSDeviceIndex(LUSDeviceIndex index);
    void UpdateControllerNamesFromConfig();

    void InitializeMappingsSinglePlayer();
    
    
    void SaveMappingIdsToConfig();
    void HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex);
    void HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId);

  private:
    bool mIsInitialized;
    LUSDeviceIndex GetLUSDeviceIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId);
    uint8_t GetPortIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId);
    int32_t GetNewSDLDeviceIndexFromLUSDeviceIndex(LUSDeviceIndex lusIndex);
    std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>>
        mLUSDeviceIndexToPhysicalDeviceIndexMappings;
    #ifdef __WIIU__
    std::unordered_map<LUSDeviceIndex, int32_t> mLUSDeviceIndexToWiiUExtensionTypes;
    #else
    std::unordered_map<LUSDeviceIndex, std::string> mLUSDeviceIndexToSDLControllerNames;
    #endif
};
} // namespace LUS
