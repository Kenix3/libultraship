#pragma once

#ifdef __WIIU__
#include "ShipDKDeviceIndexToWiiUDeviceIndexMapping.h"
#else
#include "ShipDKDeviceIndexToSDLDeviceIndexMapping.h"
#endif

#include <unordered_map>
#include <memory>
#include <vector>

namespace ShipDK {
class ShipDKDeviceIndexMappingManager {
  public:
    ShipDKDeviceIndexMappingManager();
    ~ShipDKDeviceIndexMappingManager();

#ifdef __WIIU__
    void InitializeMappingsMultiplayer(std::vector<int32_t> wiiuDeviceChannels);
    void InitializeWiiUMappingsForPort(uint8_t n64port, bool isGamepad, int32_t wiiuChannel);
    bool IsValidWiiUExtensionType(int32_t extensionType);
    void UpdateExtensionTypesFromConfig();
    std::pair<bool, int32_t> GetWiiUDeviceTypeFromShipDKDeviceIndex(ShipDKDeviceIndex index);
    void HandlePhysicalDevicesChanged();
#else
    void InitializeMappingsMultiplayer(std::vector<int32_t> sdlIndices);
    void InitializeSDLMappingsForPort(uint8_t n64port, int32_t sdlIndex);
    void UpdateControllerNamesFromConfig();
    std::string GetSDLControllerNameFromShipDKDeviceIndex(ShipDKDeviceIndex index);
    void HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex);
    void HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId);
    ShipDKDeviceIndex GetShipDKDeviceIndexFromSDLDeviceIndex(int32_t sdlIndex);
#endif

    std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping> CreateDeviceIndexMappingFromConfig(std::string id);

    std::unordered_map<ShipDKDeviceIndex, std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping>>
    GetAllDeviceIndexMappings();
    std::unordered_map<ShipDKDeviceIndex, std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping>>
    GetAllDeviceIndexMappingsFromConfig();
    std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping>
    GetDeviceIndexMappingFromShipDKDeviceIndex(ShipDKDeviceIndex lusIndex);
    void
    SetShipDKDeviceIndexToPhysicalDeviceIndexMapping(std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping> mapping);
    void RemoveShipDKDeviceIndexToPhysicalDeviceIndexMapping(ShipDKDeviceIndex index);
    ShipDKDeviceIndex GetLowestShipDKDeviceIndexWithNoAssociatedButtonOrAxisDirectionMappings(); // did this name for the meme

    void InitializeMappingsSinglePlayer();

    void SaveMappingIdsToConfig();

  private:
    bool mIsInitialized;
    std::unordered_map<ShipDKDeviceIndex, std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping>>
        mShipDKDeviceIndexToPhysicalDeviceIndexMappings;
#ifdef __WIIU__
    std::unordered_map<ShipDKDeviceIndex, std::pair<bool, int32_t>> mShipDKDeviceIndexToWiiUDeviceTypes;
#else
    std::unordered_map<ShipDKDeviceIndex, std::string> mShipDKDeviceIndexToSDLControllerNames;
    int32_t GetNewSDLDeviceIndexFromShipDKDeviceIndex(ShipDKDeviceIndex lusIndex);
    ShipDKDeviceIndex GetShipDKDeviceIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId);
    uint8_t GetPortIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId);
    void HandlePhysicalDeviceDisconnectSinglePlayer(int32_t sdlJoystickInstanceId);
    void HandlePhysicalDeviceDisconnectMultiplayer(int32_t sdlJoystickInstanceId);
#endif
};
} // namespace ShipDK
