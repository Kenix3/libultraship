#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <string>

#include "controller/controldevice/controller/mapping/ControllerRumbleMapping.h"

namespace LUS {
class ControllerRumble {
  public:
    ControllerRumble(uint8_t portIndex);
    ~ControllerRumble();

    std::unordered_map<std::string, std::shared_ptr<ControllerRumbleMapping>> GetAllRumbleMappings();
    void AddRumbleMapping(std::shared_ptr<ControllerRumbleMapping> mapping);
    void ClearRumbleMapping(std::string id);
    void SaveRumbleMappingIdsToConfig();
    void ClearAllMappings();
    void ResetToDefaultMappings(bool sdl, int32_t sdlControllerIndex);
    void LoadRumbleMappingFromConfig(std::string id);
    void ReloadAllMappingsFromConfig();

    void StartRumble();
    void StopRumble();

  private:
    uint8_t mPortIndex;
    std::unordered_map<std::string, std::shared_ptr<ControllerRumbleMapping>> mRumbleMappings;
};
} // namespace LUS
