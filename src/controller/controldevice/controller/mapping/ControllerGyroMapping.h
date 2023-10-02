#pragma once

#include <cstdint>
#include <string>
#include "ControllerInputMapping.h"

namespace LUS {
class ControllerGyroMapping : virtual public ControllerInputMapping {
  public:
    ControllerGyroMapping(uint8_t portIndex, float sensitivity);
    ~ControllerGyroMapping();
    virtual void UpdatePad(float& x, float& y) = 0;
    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;
    virtual void Recalibrate() = 0;
    virtual std::string GetGyroMappingId() = 0;
    float GetSensitivity();
    uint8_t GetSensitivityPercent();
    void SetSensitivity(uint8_t sensitivityPercent);

  protected:
    uint8_t mPortIndex;
    uint8_t mSensitivityPercent;
    float mSensitivity;
};
} // namespace LUS
