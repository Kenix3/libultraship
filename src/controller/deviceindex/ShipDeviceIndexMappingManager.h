#pragma once

#include "ShipDeviceIndexToSDLDeviceIndexMapping.h"

#include <unordered_map>
#include <memory>
#include <vector>

namespace Ship {
class ShipDeviceIndexMappingManager {
  public:
    ShipDeviceIndexMappingManager();
    ~ShipDeviceIndexMappingManager();

    void InitializeMappingsMultiplayer(std::vector<int32_t> sdlIndices);
    void InitializeSDLMappingsForPort(uint8_t n64port, int32_t sdlIndex);
    void UpdateControllerNamesFromConfig();
    std::string GetSDLControllerNameFromShipDeviceIndex(ShipDeviceIndex index);
    void HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex);
    void HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId);
    ShipDeviceIndex GetShipDeviceIndexFromSDLDeviceIndex(int32_t sdlIndex);

    std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping> CreateDeviceIndexMappingFromConfig(std::string id);

    std::unordered_map<ShipDeviceIndex, std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>>
    GetAllDeviceIndexMappings();
    std::unordered_map<ShipDeviceIndex, std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>>
    GetAllDeviceIndexMappingsFromConfig();
    std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>
    GetDeviceIndexMappingFromShipDeviceIndex(ShipDeviceIndex lusIndex);
    void SetShipDeviceIndexToPhysicalDeviceIndexMapping(
        std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping> mapping);
    void RemoveShipDeviceIndexToPhysicalDeviceIndexMapping(ShipDeviceIndex index);
    ShipDeviceIndex
    GetLowestShipDeviceIndexWithNoAssociatedButtonOrAxisDirectionMappings(); // did this name for the meme

    void InitializeMappingsSinglePlayer();

    void SaveMappingIdsToConfig();

  private:
    bool mIsInitialized;
    std::unordered_map<ShipDeviceIndex, std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>>
        mShipDeviceIndexToPhysicalDeviceIndexMappings;
    std::unordered_map<ShipDeviceIndex, std::string> mShipDeviceIndexToSDLControllerNames;
    int32_t GetNewSDLDeviceIndexFromShipDeviceIndex(ShipDeviceIndex lusIndex);
    ShipDeviceIndex GetShipDeviceIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId);
    uint8_t GetPortIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId);
    void HandlePhysicalDeviceDisconnectSinglePlayer(int32_t sdlJoystickInstanceId);
    void HandlePhysicalDeviceDisconnectMultiplayer(int32_t sdlJoystickInstanceId);
};
} // namespace Ship
