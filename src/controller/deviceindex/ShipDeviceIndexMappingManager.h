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

    void InitializeSDLMappingsForPort(uint8_t n64port);
    void UpdateControllerNamesFromConfig();
    std::string GetSDLControllerNameFromShipDeviceIndex(PhysicalDeviceType index);
    void HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex);
    void HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId);
    PhysicalDeviceType GetShipDeviceIndexFromSDLDeviceIndex(int32_t sdlIndex);

    std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping> CreateDeviceIndexMappingFromConfig(std::string id);

    std::unordered_map<PhysicalDeviceType, std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>>
    GetAllDeviceIndexMappings();
    std::unordered_map<PhysicalDeviceType, std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>>
    GetAllDeviceIndexMappingsFromConfig();
    std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>
    GetDeviceIndexMappingFromShipDeviceIndex(PhysicalDeviceType lusIndex);
    void SetShipDeviceIndexToPhysicalDeviceIndexMapping(
        std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping> mapping);
    void RemoveShipDeviceIndexToPhysicalDeviceIndexMapping(PhysicalDeviceType index);
    PhysicalDeviceType
    GetLowestShipDeviceIndexWithNoAssociatedButtonOrAxisDirectionMappings(); // did this name for the meme

    void InitializeMappingsSinglePlayer();

    void SaveMappingIdsToConfig();

  private:
    bool mIsInitialized;
    std::unordered_map<PhysicalDeviceType, std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>>
        mShipDeviceIndexToPhysicalDeviceIndexMappings;
    std::unordered_map<PhysicalDeviceType, std::string> mShipDeviceIndexToSDLControllerNames;
    int32_t GetNewSDLDeviceIndexFromShipDeviceIndex(PhysicalDeviceType lusIndex);
    PhysicalDeviceType GetShipDeviceIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId);
    uint8_t GetPortIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId);
    void HandlePhysicalDeviceDisconnectSinglePlayer(int32_t sdlJoystickInstanceId);
    void HandlePhysicalDeviceDisconnectMultiplayer(int32_t sdlJoystickInstanceId);
};
} // namespace Ship
