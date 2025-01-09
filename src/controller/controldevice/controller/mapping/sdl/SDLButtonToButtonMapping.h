#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "SDLButtonToAnyMapping.h"

namespace Ship {
class SDLButtonToButtonMapping final : public SDLButtonToAnyMapping, public ControllerButtonMapping {
  public:
    SDLButtonToButtonMapping(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                             int32_t sdlControllerButton);
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons) override;
    int8_t GetMappingType() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace Ship
