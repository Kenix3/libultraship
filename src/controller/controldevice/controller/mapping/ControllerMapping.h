#pragma once

#include <string>

namespace LUS {
class ControllerMapping {
  public:
    ControllerMapping();
    ~ControllerMapping();
    virtual std::string GetPhysicalInputName();
};
} // namespace LUS
