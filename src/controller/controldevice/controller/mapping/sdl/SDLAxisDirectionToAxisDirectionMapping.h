#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "SDLMapping.h"

namespace LUS {
class SDLAxisDirectionToAxisDirectionMapping final : public ControllerAxisDirectionMapping, public SDLMapping {
  public:
    SDLAxisDirectionToAxisDirectionMapping(uint8_t portIndex, Stick stick, Direction direction, int32_t sdlControllerIndex, int32_t sdlControllerAxis,
                                           int32_t axisDirection);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionName() override;
    std::string GetAxisDirectionMappingId() override;
    uint8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;

  private:
    SDL_GameControllerAxis mControllerAxis;
    AxisDirection mAxisDirection;
};
} // namespace LUS
