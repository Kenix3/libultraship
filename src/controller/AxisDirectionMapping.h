#pragma once

#include <cstdint>

#define MAX_AXIS_RANGE 85.0f

namespace LUS {
class AxisDirectionMapping {
  public:
    AxisDirectionMapping();
    ~AxisDirectionMapping();
    virtual float GetNormalizedAxisDirectionValue() = 0;
};
} // namespace LUS
