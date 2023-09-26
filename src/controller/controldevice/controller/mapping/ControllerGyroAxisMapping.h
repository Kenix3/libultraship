#pragma once

#include <cstdint>

#define MAX_AXIS_RANGE 85.0f

namespace LUS {
class ControllerGyroAxisMapping {
  public:
    ControllerGyroAxisMapping();
    ~ControllerGyroAxisMapping();
    virtual float GetGyroAxisValue() = 0;
};
} // namespace LUS
