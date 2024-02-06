#pragma once

#include "SDLMapping.h"
#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"

namespace LUS {
class SDLButtonToAnyMapping : virtual public ControllerInputMapping, public SDLMapping {
  public:
    SDLButtonToAnyMapping(LUSDeviceIndex lusDeviceIndex, int32_t sdlControllerButton);
    ~SDLButtonToAnyMapping();
    std::string GetPhysicalInputName() override;
    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;

  protected:
    SDL_GameControllerButton mControllerButton;

  private:
    std::string GetPlaystationButtonName();
    std::string GetSwitchButtonName();
    std::string GetXboxButtonName();
    std::string GetGameCubeButtonName();
    std::string GetGenericButtonName();
};
} // namespace LUS
