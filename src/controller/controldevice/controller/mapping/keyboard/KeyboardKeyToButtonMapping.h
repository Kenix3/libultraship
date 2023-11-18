#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "KeyboardKeyToAnyMapping.h"

namespace LUS {
class KeyboardKeyToButtonMapping final : public KeyboardKeyToAnyMapping, public ControllerButtonMapping {
  public:
    KeyboardKeyToButtonMapping(uint8_t portIndex, uint16_t bitmask, KbScancode scancode);
    void UpdatePad(uint16_t& padButtons) override;
    uint8_t GetMappingType() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace LUS
