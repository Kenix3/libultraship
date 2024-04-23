#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "SDLButtonToAnyMapping.h"

namespace LUS {
class SDLButtonToButtonMapping final : public SDLButtonToAnyMapping, public ControllerButtonMapping {
  public:
    SDLButtonToButtonMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                             int32_t sdlControllerButton);
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons) override;
    uint8_t GetMappingType() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace LUS
