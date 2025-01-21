#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "SDLButtonToAnyMapping.h"

namespace Ship {
class SDLButtonToAxisDirectionMapping final : public ControllerAxisDirectionMapping, public SDLButtonToAnyMapping {
  public:
    SDLButtonToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex, Direction direction,
                                    int32_t sdlControllerButton);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    int8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace Ship
