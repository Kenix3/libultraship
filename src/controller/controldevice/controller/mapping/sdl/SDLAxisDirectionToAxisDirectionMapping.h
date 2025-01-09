#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "SDLAxisDirectionToAnyMapping.h"

namespace Ship {
class SDLAxisDirectionToAxisDirectionMapping final : public ControllerAxisDirectionMapping,
                                                     public SDLAxisDirectionToAnyMapping {
  public:
    SDLAxisDirectionToAxisDirectionMapping(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex, StickIndex stickIndex,
                                           Direction direction, int32_t sdlControllerAxis, int32_t axisDirection);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    int8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace Ship
