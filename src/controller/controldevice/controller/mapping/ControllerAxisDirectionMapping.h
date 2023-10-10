#pragma once

#include <cstdint>
#include <string>

#include "ControllerInputMapping.h"

#define MAX_AXIS_RANGE 85.0f

namespace LUS {
enum Stick { LEFT_STICK, RIGHT_STICK };
enum Direction { LEFT, RIGHT, UP, DOWN };

class ControllerAxisDirectionMapping : virtual public ControllerInputMapping {
  public:
    ControllerAxisDirectionMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, Stick stick, Direction direction);
    ~ControllerAxisDirectionMapping();
    virtual float GetNormalizedAxisDirectionValue() = 0;
    virtual uint8_t GetMappingType();

    virtual std::string GetAxisDirectionMappingId() = 0;
    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;
    Direction GetDirection();

    void SetPortIndex(uint8_t portIndex);

  protected:
    uint8_t mPortIndex;
    Stick mStick;
    Direction mDirection;
};
} // namespace LUS
