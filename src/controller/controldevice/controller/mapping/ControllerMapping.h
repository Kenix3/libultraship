#pragma once

#include <string>
#include "controller/physicaldevice/PhysicalDeviceType.h"

namespace Ship {

#define MAPPING_TYPE_UNKNOWN -1
#define MAPPING_TYPE_GAMEPAD 0
#define MAPPING_TYPE_KEYBOARD 1
#define MAPPING_TYPE_MOUSE 2

class ControllerMapping {
  public:
    ControllerMapping(PhysicalDeviceType physicalDeviceType);
    ~ControllerMapping();
    virtual std::string GetPhysicalDeviceName();
    PhysicalDeviceType GetPhysicalDeviceType();

  protected:
    PhysicalDeviceType mPhysicalDeviceType;
};
} // namespace Ship
