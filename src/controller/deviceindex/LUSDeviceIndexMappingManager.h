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
    // std::vector<int32_t> GetDeviceOrder();

    void InitializeMappingsSinglePlayer();
    void InitializeMappingsMultiplayer(std::vector<int32_t> sdlIndices);
    void InitializeSDLMappingsForPort(uint8_t n64port, int32_t sdlIndex);
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
};
} // namespace LUS
