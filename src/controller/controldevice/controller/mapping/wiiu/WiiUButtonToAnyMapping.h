#ifdef __WIIU__
#pragma once

#include "WiiUMapping.h"
#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"

namespace LUS {
class WiiUButtonToAnyMapping : virtual public ControllerInputMapping, public WiiUMapping {
  public:
    WiiUButtonToAnyMapping(LUSDeviceIndex lusDeviceIndex, bool isNunchuk, bool isClassic, int32_t wiiuControllerButton);
    ~WiiUButtonToAnyMapping();
    std::string GetPhysicalInputName() override;
    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;

  protected:
    bool mIsNunchukButton;
    bool mIsClassicControllerButton;
    int32_t mControllerButton;

  private:
    std::string GetGamepadButtonName();
    std::string GetWiiRemoteButtonName();
    std::string GetNunchukButtonName();
    std::string GetClassicControllerButtonName();
    std::string GetProControllerButtonName();
};
} // namespace LUS
#endif
