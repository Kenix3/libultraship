#pragma once

#include <cstdint>

namespace LUS {
class ControlDevice {
  public:
    ControlDevice(uint8_t portIndex);
    virtual ~ControlDevice();

  protected:
    uint8_t mPortIndex;
};
} // namespace LUS
