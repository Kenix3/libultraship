#ifdef __WIIU__
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "controller/controldevice/controller/mapping/ControllerMapping.h"
#include <vpad/input.h>
#include <padscore/kpad.h>
#include "port/wiiu/WiiUImpl.h"

namespace LUS {
enum Axis { X = 0, Y = 1 };
enum AxisDirection { NEGATIVE = -1, POSITIVE = 1 };

class WiiUMapping : public ControllerMapping {
  public:
    WiiUMapping(LUSDeviceIndex lusDeviceIndex);
    ~WiiUMapping();

  protected:
    std::string GetWiiUDeviceName();
    int32_t GetWiiUDeviceChannel();
    bool IsGamepad();
    int32_t ExtensionType();
    std::string GetWiiUControllerName();
    bool WiiUDeviceIsConnected();
};
} // namespace LUS
#endif
