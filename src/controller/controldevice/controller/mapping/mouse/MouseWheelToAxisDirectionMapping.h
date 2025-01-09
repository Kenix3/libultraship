#pragma once

#include "MouseWheelToAnyMapping.h"
#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {
class MouseWheelToAxisDirectionMapping final : public MouseWheelToAnyMapping, public ControllerAxisDirectionMapping {
  public:
    MouseWheelToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex, Direction direction,
                                     WheelDirection wheelDirection);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    int8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace Ship
