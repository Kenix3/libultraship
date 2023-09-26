#pragma once

#include <cstdint>
#include <string>

#include "MappingTypes.h"

#define MAX_AXIS_RANGE 85.0f

namespace LUS {

class ControllerAxisDirectionMapping {
  public:
    ControllerAxisDirectionMapping();
    ControllerAxisDirectionMapping(std::string uuid);
    ~ControllerAxisDirectionMapping();
    virtual float GetNormalizedAxisDirectionValue() = 0;
    virtual uint8_t GetMappingType();
    virtual std::string GetAxisDirectionName();

    std::string GetUuid();
    virtual void SaveToConfig() = 0;

  protected:
    std::string mUuid;

  private:
    void GenerateUuid();
};
} // namespace LUS
