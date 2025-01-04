#pragma once

#include "SDLMapping.h"
#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"

namespace Ship {
class SDLButtonToAnyMapping : virtual public ControllerInputMapping, public SDLMapping {
  public:
    SDLButtonToAnyMapping(ShipDeviceIndex shipDeviceIndex, int32_t sdlControllerButton);
    ~SDLButtonToAnyMapping();
    std::string GetPhysicalInputName() override;
    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;

  protected:
    SDL_GamepadButton mControllerButton;

  private:
    std::string GetPlaystationButtonName();
    std::string GetSwitchButtonName();
    std::string GetXboxButtonName();
    std::string GetGameCubeButtonName();
    std::string GetGenericButtonName();
};
} // namespace Ship
