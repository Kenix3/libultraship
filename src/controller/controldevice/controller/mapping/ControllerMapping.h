#pragma once

#include <string>

namespace LUS {

#define MAPPING_TYPE_GAMEPAD 0
#define MAPPING_TYPE_KEYBOARD 1
#define MAPPING_TYPE_UNKNOWN 2

class ControllerMapping {
  public:
    ControllerMapping();
    ~ControllerMapping();
    virtual std::string GetPhysicalDeviceName();
};
} // namespace LUS
