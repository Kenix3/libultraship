#pragma once

#include <string>
#include "ControllerMapping.h"

namespace LUS {

class ControllerInputMapping : public ControllerMapping {
  public:
    ControllerInputMapping();
    ~ControllerInputMapping();
    virtual std::string GetPhysicalInputName();
};
} // namespace LUS
