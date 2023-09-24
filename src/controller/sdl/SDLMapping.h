#pragma once

#include <cstdint>
#include <SDL2/SDL.h>
#include <memory>

namespace LUS {
enum Axis { X = 0, Y = 1 };
enum AxisDirection { NEGATIVE = -1, POSITIVE = 1 };

class SDLMapping {
  public:
    SDLMapping(int32_t sdlControllerIndex);
    ~SDLMapping();

  protected:
    bool ControllerLoaded();

    int32_t mControllerIndex;
    SDL_GameController* mController;

  private:
    bool OpenController();
    bool CloseController();
};
} // namespace LUS