#ifdef __WIIU__
#pragma once

#include "WiiUMapping.h"
#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"

namespace LUS {
class WiiUButtonToAnyMapping : virtual public ControllerInputMapping, public WiiUMapping {
  public:
    WiiUButtonToAnyMapping(LUSDeviceIndex lusDeviceIndex, int32_t wiiuControllerButton);
    ~WiiUButtonToAnyMapping();
    std::string GetPhysicalInputName() override;
    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;

  protected:
    int32_t mControllerButton;
};
} // namespace LUS
#endif
