#pragma once

#include <cstdint>
#include <memory>

#include "controller/controldevice/controller/mapping/ControllerGyroMapping.h"

namespace LUS {
class ControllerGyro {
  public:
    ControllerGyro(uint8_t portIndex);
    ~ControllerGyro();

    // void AddOrReplaceGyroMapping(std::shared_ptr<ControllerGyroMapping> mapping);
    void ReloadGyroMappingFromConfig();
    void ClearGyroMapping();
    void SaveGyroMappingIdToConfig();

    std::shared_ptr<ControllerGyroMapping> GetGyroMapping();

    void UpdatePad(float& x, float& y);

  private:
    uint8_t mPortIndex;
    std::shared_ptr<ControllerGyroMapping> mGyroMapping;
};
} // namespace LUS
