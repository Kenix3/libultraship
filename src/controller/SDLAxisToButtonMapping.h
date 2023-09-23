#include "ButtonMapping.h"
#include "SDLMapping.h"

namespace LUS {
enum AxisDirection { NEGATIVE = -1, POSITIVE = 1};

class SDLAxisToButtonMapping final : public ButtonMapping, public SDLMapping {
  public:
    SDLAxisToButtonMapping(uint16_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerAxis, int32_t axisDirection);
    void UpdatePad(uint16_t& padButtons) override;

  private:
    SDL_GameControllerAxis mControllerAxis;
    AxisDirection mAxisDirection;
};
} // namespace LUS
