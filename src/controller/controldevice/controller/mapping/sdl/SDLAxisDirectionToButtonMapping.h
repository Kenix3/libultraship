#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "SDLAxisDirectionToAnyMapping.h"

namespace Ship {
class SDLAxisDirectionToButtonMapping final : public ControllerButtonMapping, public SDLAxisDirectionToAnyMapping {
  public:
    SDLAxisDirectionToButtonMapping(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                    int32_t sdlControllerAxis, int32_t axisDirection);
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons) override;
    int8_t GetMappingType() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace Ship
