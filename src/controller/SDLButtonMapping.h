#include "ButtonMapping.h"
#include <SDL2/SDL.h>

namespace LUS {
class SDLButtonMapping final : public ButtonMapping {
  public:
    SDLButtonMapping(int32_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerButton);
    void UpdatePad(int32_t& padButtons) override;

  private:
    int32_t mControllerIndex;
    int32_t mControllerButton;
    SDL_GameController* mController;
};
} // namespace LUS
