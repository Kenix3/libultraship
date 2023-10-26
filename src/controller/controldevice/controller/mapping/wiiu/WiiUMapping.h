#ifdef __WIIU__
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "controller/controldevice/controller/mapping/ControllerMapping.h"

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

  private:
    bool mDeviceConnected;
    std::string GetWiiUControllerName();
    bool IsGamepad();
};
} // namespace LUS
#endif
