#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <string>

#include "controller/controldevice/controller/mapping/ControllerRumbleMapping.h"

namespace Ship {
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
    void ClearAllMappingsForDeviceType(ShipDeviceType shipDeviceType);
    void AddDefaultMappings(ShipDeviceType shipDeviceType);
    void LoadRumbleMappingFromConfig(std::string id);
    void ReloadAllMappingsFromConfig();

    bool AddRumbleMappingFromRawPress();

    void StartRumble();
    void StopRumble();

    bool HasMappingsForShipDeviceType(ShipDeviceType shipDeviceType);

  private:
    uint8_t mPortIndex;
    std::unordered_map<std::string, std::shared_ptr<ControllerRumbleMapping>> mRumbleMappings;
};
} // namespace Ship
