#pragma once

#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {
class MouseButtonToAnyMapping : virtual public ControllerInputMapping {
  public:
    MouseButtonToAnyMapping(MouseBtn button);
    virtual ~MouseButtonToAnyMapping();
    bool ProcessMouseButtonEvent(bool isPressed, MouseBtn button);
    std::string GetPhysicalDeviceName() override;
    std::string GetPhysicalInputName() override;

  protected:
    MouseBtn mButton;
    bool mKeyPressed;
};
} // namespace Ship
