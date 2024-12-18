#pragma once

#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "MouseKeyToAnyMapping.h"
#include "window/MouseMeta.h"

namespace Ship {
class MouseKeyToAxisDirectionMapping final : public MouseKeyToAnyMapping, public ControllerAxisDirectionMapping {
  public:
    MouseKeyToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex, Direction direction, MouseBtn button);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    uint8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace Ship
