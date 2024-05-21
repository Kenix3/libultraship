#pragma once

#include <cstdint>
#include <SDL2/SDL.h>
#include <memory>
#include <string>
#include "controller/controldevice/controller/mapping/ControllerMapping.h"

namespace Ship {
enum Axis { X = 0, Y = 1 };
enum AxisDirection { NEGATIVE = -1, POSITIVE = 1 };

class SDLMapping : public ControllerMapping {
  public:
    SDLMapping(ShipDeviceIndex shipDeviceIndex);
    ~SDLMapping();
    int32_t GetJoystickInstanceId();
    int32_t GetCurrentSDLDeviceIndex();
    bool CloseController();
    bool ControllerLoaded(bool closeIfDisconnected = false);

  protected:
    SDL_GameControllerType GetSDLControllerType();
    uint16_t GetSDLControllerVendorId();
    uint16_t GetSDLControllerProductId();
    bool UsesPlaystationLayout();
    bool UsesSwitchLayout();
    bool UsesXboxLayout();
    bool UsesGameCubeLayout();
    std::string GetSDLDeviceName();
    int32_t GetSDLDeviceIndex();

    SDL_GameController* mController;

  private:
    bool OpenController();
    std::string GetSDLControllerName();
};
} // namespace Ship
