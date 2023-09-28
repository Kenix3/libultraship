#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "KeyboardKeyToAnyMapping.h"

namespace LUS {
class KeyboardKeyToAxisDirectionMapping final : public KeyboardKeyToAnyMapping, public ControllerAxisDirectionMapping {
  public:
    KeyboardKeyToAxisDirectionMapping(uint8_t portIndex, Stick stick, Direction direction, KbScancode scancode);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    uint8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace LUS
