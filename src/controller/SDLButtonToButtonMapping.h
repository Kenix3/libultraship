#include "ButtonMapping.h"
#include "SDLMapping.h"

namespace LUS {
class SDLButtonToButtonMapping final : public ButtonMapping, public SDLMapping {
  public:
    SDLButtonToButtonMapping(int32_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerButton);
    void UpdatePad(int32_t& padButtons) override;

  private:
    SDL_GameControllerButton mControllerButton;
};
} // namespace LUS
