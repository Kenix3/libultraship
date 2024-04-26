#pragma once

#include <string>
#include "ControllerMapping.h"

namespace ShipDK {

class ControllerInputMapping : public ControllerMapping {
  public:
    ControllerInputMapping(ShipDKDeviceIndex shipDKDeviceIndex);
    ~ControllerInputMapping();
    virtual std::string GetPhysicalInputName();
};
} // namespace ShipDK
