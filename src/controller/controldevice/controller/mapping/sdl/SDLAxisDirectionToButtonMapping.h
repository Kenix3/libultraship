#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "SDLAxisDirectionToAnyMapping.h"

namespace LUS {
class SDLAxisDirectionToButtonMapping final : public ControllerButtonMapping, public SDLAxisDirectionToAnyMapping {
  public:
    SDLAxisDirectionToButtonMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, uint16_t bitmask,
                                    int32_t sdlControllerAxis, int32_t axisDirection);
    void UpdatePad(uint16_t& padButtons) override;
    uint8_t GetMappingType() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace LUS
