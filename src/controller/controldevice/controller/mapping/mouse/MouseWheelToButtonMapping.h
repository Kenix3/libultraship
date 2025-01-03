#pragma once

#include "MouseWheelToAnyMapping.h"
#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {
class MouseWheelToButtonMapping final : public MouseWheelToAnyMapping, public ControllerButtonMapping {
  public:
    MouseWheelToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, WheelDirection wheelDirection);
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons) override;
    int8_t GetMappingType() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace Ship
