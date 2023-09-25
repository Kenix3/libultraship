#pragma once

#include <cstdint>
#include <string>

#include "MappingTypes.h"

#define MAX_AXIS_RANGE 85.0f

namespace LUS {

class AxisDirectionMapping {
  public:
    AxisDirectionMapping();
    AxisDirectionMapping(std::string uuid);
    ~AxisDirectionMapping();
    virtual float GetNormalizedAxisDirectionValue() = 0;
    virtual uint8_t GetMappingType();
    virtual std::string GetAxisDirectionName();
  
  protected:
    std::string mUuid;
  
  private:
    void GenerateUuid();
};
} // namespace LUS
