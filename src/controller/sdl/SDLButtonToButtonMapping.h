#include "controller/ButtonMapping.h"
#include "SDLMapping.h"

namespace LUS {
class SDLButtonToButtonMapping final : public ButtonMapping, public SDLMapping {
  public:
    SDLButtonToButtonMapping(uint16_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerButton);
    void UpdatePad(uint16_t& padButtons) override;

  private:
    SDL_GameControllerButton mControllerButton;
};
} // namespace LUS
