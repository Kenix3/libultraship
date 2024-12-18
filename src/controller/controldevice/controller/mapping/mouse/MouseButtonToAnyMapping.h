#pragma once

#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {
class MouseButtonToAnyMapping : virtual public ControllerInputMapping {
  public:
    MouseButtonToAnyMapping(MouseBtn button);
    ~MouseButtonToAnyMapping();
    std::string GetPhysicalInputName() override;
    bool ProcessMouseEvent(bool isPressed, MouseBtn button);
    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;

  protected:
    MouseBtn mButton;
    bool mKeyPressed;
};
} // namespace Ship
