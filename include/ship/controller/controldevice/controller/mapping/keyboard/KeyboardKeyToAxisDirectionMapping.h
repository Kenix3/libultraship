#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "KeyboardKeyToAnyMapping.h"

namespace Ship {
class KeyboardKeyToAxisDirectionMapping final : public KeyboardKeyToAnyMapping, public ControllerAxisDirectionMapping {
  public:
    KeyboardKeyToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex, Direction direction,
                                      KbScancode scancode);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    int8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
    std::string GetPhysicalDeviceName() override;
    std::string GetPhysicalInputName() override;
};
} // namespace Ship
