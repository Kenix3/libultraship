#pragma once

#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "MouseButtonToAnyMapping.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {
class MouseButtonToButtonMapping final : public MouseButtonToAnyMapping, public ControllerButtonMapping {
  public:
    MouseButtonToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, MouseBtn button);
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons) override;
    uint8_t GetMappingType() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace Ship
