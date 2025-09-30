#pragma once

#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"
#include "KeyboardScancodes.h"

namespace Ship {
class KeyboardKeyToAnyMapping : virtual public ControllerInputMapping {
  public:
    KeyboardKeyToAnyMapping(KbScancode scancode);
    virtual ~KeyboardKeyToAnyMapping();
    bool ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode);
    std::string GetPhysicalDeviceName() override;
    std::string GetPhysicalInputName() override;

  protected:
    KbScancode mKeyboardScancode;
    bool mKeyPressed;
};
} // namespace Ship
