#pragma once

#include "controller/controldevice/controller/mapping/ControllerMapping.h"
#include "KeyboardScancodes.h"

namespace LUS {
class KeyboardKeyToAnyMapping : virtual public ControllerMapping {
  public:
    KeyboardKeyToAnyMapping(KbScancode scancode);
    ~KeyboardKeyToAnyMapping();
    std::string GetPhysicalInputName() override;
    bool ProcessKeyboardEvent(LUS::KbEventType eventType, LUS::KbScancode scancode);

  protected:
    KbScancode mKeyboardScancode;
    bool mKeyPressed;
};
} // namespace LUS
