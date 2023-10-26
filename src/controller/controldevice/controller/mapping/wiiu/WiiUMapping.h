#ifdef __WIIU__
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "controller/controldevice/controller/mapping/ControllerMapping.h"
#include <vpad/input.h>
#include <padscore/kpad.h>

namespace LUS {
enum Axis { X = 0, Y = 1 };
enum AxisDirection { NEGATIVE = -1, POSITIVE = 1 };

class WiiUMapping : public ControllerMapping {
  public:
    WiiUMapping(LUSDeviceIndex lusDeviceIndex);
    ~WiiUMapping();

    bool OpenController();
    bool CloseController();
    bool ControllerLoaded();

  protected:
    std::string GetWiiUDeviceName();
    int32_t GetWiiUDeviceChannel();
    bool IsGamepad();
    int32_t ExtensionType();

    bool mDeviceConnected;
    std::string GetWiiUControllerName();

    KPADStatus* mController;
    VPADStatus* mWiiUGamepadController;
};
} // namespace LUS
#endif
