#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <string>

#include "controller/controldevice/controller/mapping/ControllerRumbleMapping.h"

namespace ShipDK {
class ControllerRumble {
  public:
    ControllerRumble(uint8_t portIndex);
    ~ControllerRumble();

    std::unordered_map<std::string, std::shared_ptr<ControllerRumbleMapping>> GetAllRumbleMappings();
    void AddRumbleMapping(std::shared_ptr<ControllerRumbleMapping> mapping);
    void ClearRumbleMappingId(std::string id);
    void ClearRumbleMapping(std::string id);
    void SaveRumbleMappingIdsToConfig();
    void ClearAllMappings();
    void ClearAllMappingsForDevice(ShipDKDeviceIndex shipDKDeviceIndex);
    void AddDefaultMappings(ShipDKDeviceIndex shipDKDeviceIndex);
    void LoadRumbleMappingFromConfig(std::string id);
    void ReloadAllMappingsFromConfig();

    bool AddRumbleMappingFromRawPress();

    void StartRumble();
    void StopRumble();

    bool HasMappingsForShipDKDeviceIndex(ShipDKDeviceIndex lusIndex);

  private:
    uint8_t mPortIndex;
    std::unordered_map<std::string, std::shared_ptr<ControllerRumbleMapping>> mRumbleMappings;
};
} // namespace ShipDK
