#pragma once

#include <cstdint>
#include <string>

#include "MappingTypes.h"

#define MAX_AXIS_RANGE 85.0f

namespace LUS {

class AxisDirectionMapping {
  public:
    AxisDirectionMapping();
    ~AxisDirectionMapping();
    virtual float GetNormalizedAxisDirectionValue() = 0;
    virtual uint8_t GetMappingType();
    virtual std::string GetAxisDirectionName();
};
} // namespace LUS
