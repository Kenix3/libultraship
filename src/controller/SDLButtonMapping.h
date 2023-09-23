#include "ButtonMapping.h"
#include "SDLMapping.h"

namespace LUS {
class SDLButtonMapping final : public ButtonMapping, public SDLMapping {
  public:
    SDLButtonMapping(int32_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerButton);
    void UpdatePad(int32_t& padButtons) override;

  private:
    int32_t mControllerButton;
};
} // namespace LUS
