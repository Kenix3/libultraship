#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "KeyboardKeyToAnyMapping.h"

namespace Ship {
class KeyboardKeyToButtonMapping final : public KeyboardKeyToAnyMapping, public ControllerButtonMapping {
  public:
    KeyboardKeyToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, KbScancode scancode);
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons) override;
    int8_t GetMappingType() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace Ship
