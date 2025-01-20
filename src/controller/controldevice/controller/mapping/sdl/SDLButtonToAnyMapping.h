#pragma once

#include "SDLMapping.h"
#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"

namespace Ship {
class SDLButtonToAnyMapping : virtual public ControllerInputMapping {
  public:
    SDLButtonToAnyMapping(int32_t sdlControllerButton);
    ~SDLButtonToAnyMapping();
    std::string GetPhysicalInputName() override;
    std::string GetPhysicalDeviceName() override;

  protected:
    SDL_GameControllerButton mControllerButton;

  private:
    std::string GetGenericButtonName();
};
} // namespace Ship
