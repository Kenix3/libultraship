#include "controller/GyroAxisMapping.h"
#include "SDLMapping.h"

namespace LUS {
class SDLGyroAxisMapping final : public GyroAxisMapping, public SDLMapping {
  public:
    SDLGyroAxisMapping(int32_t sdlControllerIndex, int32_t axis);
    float GetGyroAxisValue() override;

  private:
    Axis mAxis;
};
} // namespace LUS
