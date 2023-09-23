#include <cstdint>
#include <SDL2/SDL.h>
#include <memory>

namespace LUS {
class SDLMapping {
  public:
    SDLMapping(int32_t sdlControllerIndex);
    ~SDLMapping();

  protected:
    bool OpenController();
    bool CloseController();

    int32_t mControllerIndex;
    SDL_GameController* mController;
};
} // namespace LUS