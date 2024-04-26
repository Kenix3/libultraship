#pragma once

#include <string>
#include "controller/deviceindex/ShipDKDeviceIndex.h"

namespace ShipDK {

#define MAPPING_TYPE_GAMEPAD 0
#define MAPPING_TYPE_KEYBOARD 1
#define MAPPING_TYPE_UNKNOWN 2

class ControllerMapping {
  public:
    ControllerMapping(ShipDKDeviceIndex shipDKDeviceIndex);
    ~ControllerMapping();
    virtual std::string GetPhysicalDeviceName();
    ShipDKDeviceIndex GetShipDKDeviceIndex();
    virtual bool PhysicalDeviceIsConnected() = 0;

  protected:
    ShipDKDeviceIndex mShipDKDeviceIndex;
};
} // namespace ShipDK
