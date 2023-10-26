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
    void CloseController();
    bool ControllerConnected();

  private:
    bool mDeviceConnected;
};
} // namespace LUS
#endif
