#pragma once

#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"
#include "KeyboardScancodes.h"

namespace Ship {
class KeyboardKeyToAnyMapping : virtual public ControllerInputMapping {
  public:
    KeyboardKeyToAnyMapping(KbScancode scancode);
    virtual ~KeyboardKeyToAnyMapping();
    std::string GetPhysicalInputName() override;
    bool ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode);
    std::string GetPhysicalDeviceName() override;

  protected:
    KbScancode mKeyboardScancode;
    bool mKeyPressed;
};
} // namespace Ship
