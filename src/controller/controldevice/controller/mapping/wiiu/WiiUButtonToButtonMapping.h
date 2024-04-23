#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "WiiUButtonToAnyMapping.h"

namespace LUS {
class WiiUButtonToButtonMapping final : public WiiUButtonToAnyMapping, public ControllerButtonMapping {
  public:
    WiiUButtonToButtonMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                              bool isNunchuk, bool isClassic, uint32_t wiiuControllerButton);
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons) override;
    uint8_t GetMappingType() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace LUS
#endif
