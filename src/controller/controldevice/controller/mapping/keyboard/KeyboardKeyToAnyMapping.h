#pragma once

#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"
#include "KeyboardScancodes.h"

namespace ShipDK {
class KeyboardKeyToAnyMapping : virtual public ControllerInputMapping {
  public:
    KeyboardKeyToAnyMapping(KbScancode scancode);
    ~KeyboardKeyToAnyMapping();
    std::string GetPhysicalInputName() override;
    bool ProcessKeyboardEvent(ShipDK::KbEventType eventType, ShipDK::KbScancode scancode);
    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;

  protected:
    KbScancode mKeyboardScancode;
    bool mKeyPressed;
};
} // namespace ShipDK
