#pragma once

#include <string>
#include "ControllerMapping.h"

namespace Ship {

class ControllerInputMapping : public ControllerMapping {
  public:
    ControllerInputMapping(PhysicalDeviceType physicalDeviceType);
    ~ControllerInputMapping();
    virtual std::string GetPhysicalInputName();
};
} // namespace Ship
