#pragma once

#include <cstdint>
#include <SDL2/SDL.h>
#include <memory>
#include <string>

namespace LUS {
enum Axis { X = 0, Y = 1 };
enum AxisDirection { NEGATIVE = -1, POSITIVE = 1 };

class SDLMapping {
  public:
    SDLMapping(int32_t sdlControllerIndex);
    ~SDLMapping();

  protected:
    bool ControllerLoaded();
    SDL_GameControllerType GetSDLControllerType();
    bool UsesPlaystationLayout();
    bool UsesSwitchLayout();
    bool UsesXboxLayout();
    std::string GetSDLDeviceName();
    int32_t GetSDLDeviceIndex();

    int32_t mControllerIndex;
    SDL_GameController* mController;

  private:
    bool OpenController();
    bool CloseController();
};
} // namespace LUS