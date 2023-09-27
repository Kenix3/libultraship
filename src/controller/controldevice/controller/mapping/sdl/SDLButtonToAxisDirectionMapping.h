#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "SDLButtonToAnyMapping.h"

namespace LUS {
class SDLButtonToAxisDirectionMapping final : public ControllerAxisDirectionMapping, public SDLButtonToAnyMapping {
  public:
    SDLButtonToAxisDirectionMapping(uint8_t portIndex, Stick stick, Direction direction, int32_t sdlControllerIndex, int32_t sdlControllerButton);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    uint8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace LUS
