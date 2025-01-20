#pragma once

#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {
class MouseWheelToAnyMapping : virtual public ControllerInputMapping {
  public:
    MouseWheelToAnyMapping(WheelDirection wheelDirection);
    ~MouseWheelToAnyMapping();
    std::string GetPhysicalInputName() override;
    std::string GetPhysicalDeviceName() override;

  protected:
    WheelDirection mWheelDirection;
};
} // namespace Ship
