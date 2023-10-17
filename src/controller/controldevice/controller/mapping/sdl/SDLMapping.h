#pragma once

#include <cstdint>
#include <SDL2/SDL.h>
#include <memory>
#include <string>
#include "controller/controldevice/controller/mapping/ControllerMapping.h"

namespace LUS {
enum Axis { X = 0, Y = 1 };
enum AxisDirection { NEGATIVE = -1, POSITIVE = 1 };

class SDLMapping : public ControllerMapping {
  public:
    SDLMapping(LUSDeviceIndex lusDeviceIndex);
    ~SDLMapping();
    int32_t GetJoystickInstanceId();
    int32_t GetCurrentSDLDeviceIndex();
    bool CloseController();

  protected:
    bool ControllerLoaded();
    SDL_GameControllerType GetSDLControllerType();
    bool UsesPlaystationLayout();
    bool UsesSwitchLayout();
    bool UsesXboxLayout();
    std::string GetSDLDeviceName();
    int32_t GetSDLDeviceIndex();

    SDL_GameController* mController;

  private:
    bool OpenController();
};
} // namespace LUS
