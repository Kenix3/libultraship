#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "SDLButtonToAnyMapping.h"

namespace ShipDK {
class SDLButtonToButtonMapping final : public SDLButtonToAnyMapping, public ControllerButtonMapping {
  public:
    SDLButtonToButtonMapping(ShipDKDeviceIndex shipDKDeviceIndex, uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                             int32_t sdlControllerButton);
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons) override;
    uint8_t GetMappingType() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace ShipDK
