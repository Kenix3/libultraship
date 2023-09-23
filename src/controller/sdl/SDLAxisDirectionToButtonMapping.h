#include "controller/ButtonMapping.h"
#include "SDLMapping.h"

namespace LUS {
class SDLAxisDirectionToButtonMapping final : public ButtonMapping, public SDLMapping {
  public:
    SDLAxisDirectionToButtonMapping(uint16_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerAxis,
                                    int32_t axisDirection);
    void UpdatePad(uint16_t& padButtons) override;

  private:
    SDL_GameControllerAxis mControllerAxis;
    AxisDirection mAxisDirection;
};
} // namespace LUS
