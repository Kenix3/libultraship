#pragma once

#include <string>
#include "controller/deviceindex/ShipDeviceType.h"

namespace Ship {

#define MAPPING_TYPE_UNKNOWN -1
#define MAPPING_TYPE_GAMEPAD 0
#define MAPPING_TYPE_KEYBOARD 1
#define MAPPING_TYPE_MOUSE 2

class ControllerMapping {
  public:
    ControllerMapping(ShipDeviceType shipDeviceType);
    ~ControllerMapping();
    virtual std::string GetPhysicalDeviceName();
    ShipDeviceType GetShipDeviceType();
    virtual bool PhysicalDeviceIsConnected() = 0;

  protected:
    ShipDeviceType mShipDeviceType;
};
} // namespace Ship
