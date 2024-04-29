#ifdef __WIIU__
#pragma once

#include "WiiUMapping.h"
#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"

namespace Ship {
class WiiUButtonToAnyMapping : virtual public ControllerInputMapping, public WiiUMapping {
  public:
    WiiUButtonToAnyMapping(ShipDeviceIndex shipDeviceIndex, bool isNunchuk, bool isClassic,
                           uint32_t wiiuControllerButton);
    ~WiiUButtonToAnyMapping();
    std::string GetPhysicalInputName() override;
    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;

  protected:
    bool mIsNunchukButton;
    bool mIsClassicControllerButton;
    uint32_t mControllerButton;
    bool PhysicalButtonIsPressed();

  private:
    std::string GetGamepadButtonName();
    std::string GetWiiRemoteButtonName();
    std::string GetNunchukButtonName();
    std::string GetClassicControllerButtonName();
    std::string GetProControllerButtonName();
};
} // namespace Ship
#endif
