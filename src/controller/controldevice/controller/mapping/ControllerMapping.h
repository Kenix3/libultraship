#pragma once

#include <string>
#include "controller/deviceindex/LUSDeviceIndex.h"

namespace LUS {

#define MAPPING_TYPE_GAMEPAD 0
#define MAPPING_TYPE_KEYBOARD 1
#define MAPPING_TYPE_UNKNOWN 2

class ControllerMapping {
  public:
    ControllerMapping(LUSDeviceIndex lusDeviceIndex);
    ~ControllerMapping();
    virtual std::string GetPhysicalDeviceName();
    LUSDeviceIndex GetLUSDeviceIndex();
    virtual bool PhysicalDeviceIsConnected() = 0;

  protected:
    LUSDeviceIndex mLUSDeviceIndex;
};
} // namespace LUS
