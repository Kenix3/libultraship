#pragma once

#include <cstdint>

namespace LUS {
class StickMapping {
  public:
    StickMapping();
    ~StickMapping();

    virtual void UpdatePad(int8_t& x, int8_t& y) = 0;
};
} // namespace LUS
