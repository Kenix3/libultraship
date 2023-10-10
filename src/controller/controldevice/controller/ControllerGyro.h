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
    void SetGyroMapping(std::shared_ptr<ControllerGyroMapping> mapping);
    bool SetGyroMappingFromRawPress();

    void UpdatePad(float& x, float& y);

    bool HasMappingForLUSDeviceIndex(LUSDeviceIndex lusIndex);

  private:
    uint8_t mPortIndex;
    std::shared_ptr<ControllerGyroMapping> mGyroMapping;
};
} // namespace LUS
