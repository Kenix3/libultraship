#pragma once

#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"
#include "KeyboardScancodes.h"

namespace LUS {
class KeyboardKeyToAnyMapping : virtual public ControllerInputMapping {
  public:
    KeyboardKeyToAnyMapping(KbScancode scancode);
    ~KeyboardKeyToAnyMapping();
    std::string GetPhysicalInputName() override;
    bool ProcessKeyboardEvent(LUS::KbEventType eventType, LUS::KbScancode scancode);
    std::string GetPhysicalDeviceName() override;

  protected:
    KbScancode mKeyboardScancode;
    bool mKeyPressed;
};
} // namespace LUS
