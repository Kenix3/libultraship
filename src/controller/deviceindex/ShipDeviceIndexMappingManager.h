#pragma once

#ifdef __WIIU__
#include "ShipDeviceIndexToWiiUDeviceIndexMapping.h"
#else
#include "ShipDeviceIndexToSDLDeviceIndexMapping.h"
#endif

#include <unordered_map>
#include <memory>
#include <vector>

namespace Ship {
class ShipDeviceIndexMappingManager {
  public:
    ShipDeviceIndexMappingManager();
    ~ShipDeviceIndexMappingManager();

#ifdef __WIIU__
    void InitializeMappingsMultiplayer(std::vector<int32_t> wiiuDeviceChannels);
    void InitializeWiiUMappingsForPort(uint8_t n64port, bool isGamepad, int32_t wiiuChannel);
    bool IsValidWiiUExtensionType(int32_t extensionType);
    void UpdateExtensionTypesFromConfig();
    std::pair<bool, int32_t> GetWiiUDeviceTypeFromShipDeviceIndex(ShipDeviceIndex index);
    void HandlePhysicalDevicesChanged();
#else
    void InitializeMappingsMultiplayer(std::vector<int32_t> sdlIndices);
    void InitializeSDLMappingsForPort(uint8_t n64port, int32_t sdlIndex);
    void UpdateControllerNamesFromConfig();
    std::string GetSDLControllerNameFromShipDeviceIndex(ShipDeviceIndex index);
    void HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex);
    void HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId);
    ShipDeviceIndex GetShipDeviceIndexFromSDLDeviceIndex(int32_t sdlIndex);
#endif

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
#ifdef __WIIU__
    std::unordered_map<ShipDeviceIndex, std::pair<bool, int32_t>> mShipDeviceIndexToWiiUDeviceTypes;
#else
    std::unordered_map<ShipDeviceIndex, std::string> mShipDeviceIndexToSDLControllerNames;
    int32_t GetNewSDLDeviceIndexFromShipDeviceIndex(ShipDeviceIndex lusIndex);
    ShipDeviceIndex GetShipDeviceIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId);
    uint8_t GetPortIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId);
    void HandlePhysicalDeviceDisconnectSinglePlayer(int32_t sdlJoystickInstanceId);
    void HandlePhysicalDeviceDisconnectMultiplayer(int32_t sdlJoystickInstanceId);
#endif
};
} // namespace Ship
