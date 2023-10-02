#pragma once

#include <cstdint>
#include <string>
#include "ControllerInputMapping.h"

namespace LUS {
class ControllerGyroMapping : virtual public ControllerInputMapping {
  public:
    ControllerGyroMapping(uint8_t portIndex);
    ~ControllerGyroMapping();
    virtual void UpdatePad(float& x, float& y) = 0;
    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;
    virtual std::string GetGyroMappingId() = 0;

  protected:
    uint8_t mPortIndex;
};
} // namespace LUS
