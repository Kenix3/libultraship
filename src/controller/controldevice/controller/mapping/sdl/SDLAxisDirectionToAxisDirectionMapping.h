#include "controller/AxisDirectionMapping.h"
#include "SDLMapping.h"

namespace LUS {
class SDLAxisDirectionToAxisDirectionMapping final : public AxisDirectionMapping, public SDLMapping {
  public:
    SDLAxisDirectionToAxisDirectionMapping(int32_t sdlControllerIndex, int32_t sdlControllerAxis,
                                           int32_t axisDirection);
    SDLAxisDirectionToAxisDirectionMapping(std::string uuid, int32_t sdlControllerIndex, int32_t sdlControllerAxis,
                                           int32_t axisDirection);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionName() override;
    uint8_t GetMappingType() override;
    void SaveToConfig() override;

  private:
    SDL_GameControllerAxis mControllerAxis;
    AxisDirection mAxisDirection;
};
} // namespace LUS
