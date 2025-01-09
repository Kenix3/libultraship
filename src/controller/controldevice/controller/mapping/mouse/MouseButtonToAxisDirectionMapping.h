#pragma once

#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "MouseButtonToAnyMapping.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {
class MouseButtonToAxisDirectionMapping final : public MouseButtonToAnyMapping, public ControllerAxisDirectionMapping {
  public:
    MouseButtonToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex, Direction direction, MouseBtn button);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    int8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace Ship
