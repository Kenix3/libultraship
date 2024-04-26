#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "SDLAxisDirectionToAnyMapping.h"

namespace ShipDK {
class SDLAxisDirectionToAxisDirectionMapping final : public ControllerAxisDirectionMapping,
                                                     public SDLAxisDirectionToAnyMapping {
  public:
    SDLAxisDirectionToAxisDirectionMapping(ShipDKDeviceIndex shipDKDeviceIndex, uint8_t portIndex, Stick stick,
                                           Direction direction, int32_t sdlControllerAxis, int32_t axisDirection);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    uint8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace ShipDK
