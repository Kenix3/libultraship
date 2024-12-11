#pragma once

#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"
#include "window/Window.h"

namespace Ship {
class MouseKeyToAnyMapping : virtual public ControllerInputMapping {
  public:
    MouseKeyToAnyMapping(MouseBtn button);
    ~MouseKeyToAnyMapping();
    std::string GetPhysicalInputName() override;
    bool ProcessMouseEvent(bool isPressed, MouseBtn button);
    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;

  protected:
    MouseBtn mButton;
    bool mKeyPressed;
};
} // namespace Ship
