#pragma once

#include "SDLMapping.h"
#include "controller/controldevice/controller/mapping/ControllerMapping.h"

namespace LUS {
class SDLButtonToAnyMapping : virtual public ControllerMapping, public SDLMapping {
  public:
    SDLButtonToAnyMapping(int32_t sdlControllerIndex, int32_t sdlControllerButton);
    ~SDLButtonToAnyMapping();
    std::string GetPhysicalInputName() override;

  protected:
    SDL_GameControllerButton mControllerButton;

  private:
    std::string GetPlaystationButtonName();
    std::string GetSwitchButtonName();
    std::string GetXboxButtonName();
    std::string GetGenericButtonName();
};
} // namespace LUS
