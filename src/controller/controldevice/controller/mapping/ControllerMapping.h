#pragma once

#include <string>
#include "controller/deviceindex/ShipDeviceIndex.h"

namespace Ship {

#define MAPPING_TYPE_GAMEPAD 0
#define MAPPING_TYPE_KEYBOARD 1
#define MAPPING_TYPE_MOUSE 2
#define MAPPING_TYPE_UNKNOWN 3

class ControllerMapping {
  public:
    ControllerMapping(ShipDeviceIndex shipDeviceIndex);
    ~ControllerMapping();
    virtual std::string GetPhysicalDeviceName();
    ShipDeviceIndex GetShipDeviceIndex();
    virtual bool PhysicalDeviceIsConnected() = 0;

  protected:
    ShipDeviceIndex mShipDeviceIndex;
};
} // namespace Ship
