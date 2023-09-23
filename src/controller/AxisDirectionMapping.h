#pragma once

#include <cstdint>

#define MAX_AXIS_RANGE 85.0f

namespace LUS {
class AxisDirectionMapping {
  public:
    AxisDirectionMapping();
    ~AxisDirectionMapping();
    float GetNormalizedAxisDirectionValue();
};
} // namespace LUS
