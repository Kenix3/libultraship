#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"

namespace LUS {
class KeyboardKeyToButtonMapping final : public ControllerButtonMapping {
  public:
    KeyboardKeyToButtonMapping(uint8_t portIndex, uint16_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerButton);
    void UpdatePad(uint16_t& padButtons) override;
    uint8_t GetMappingType() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace LUS