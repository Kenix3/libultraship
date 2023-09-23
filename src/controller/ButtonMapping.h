#pragma once

#include <cstdint>

namespace LUS {
class ButtonMapping {
  public:
    ButtonMapping(uint16_t bitmask);
    ~ButtonMapping();

    uint16_t GetBitmask();
    virtual void UpdatePad(uint16_t& padButtons) = 0;

  protected:
    uint16_t mBitmask;
};
} // namespace LUS