#include "controller/AxisDirectionMapping.h"
#include "SDLMapping.h"

namespace LUS {
enum AxisDirection { NEGATIVE = -1, POSITIVE = 1 };

class SDLAxisDirectionToAxisDirectionMapping final : public AxisDirectionMapping, public SDLMapping {
  public:
    SDLAxisDirectionToAxisDirectionMapping(int32_t sdlControllerIndex, int32_t sdlControllerAxis,
                                           int32_t axisDirection);
    GetNormalizedAxisDirectionValue();
  private:
    SDL_GameControllerAxis mControllerAxis;
    AxisDirection mAxisDirection;
};
} // namespace LUS
