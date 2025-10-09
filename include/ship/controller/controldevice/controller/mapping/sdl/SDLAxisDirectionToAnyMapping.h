#pragma once

#include "ship/controller/controldevice/controller/mapping/sdl/SDLMapping.h"
#include "ship/controller/controldevice/controller/mapping/ControllerInputMapping.h"

namespace Ship {
class SDLAxisDirectionToAnyMapping : virtual public ControllerInputMapping {
  public:
    SDLAxisDirectionToAnyMapping(int32_t sdlControllerAxis, int32_t axisDirection);
    virtual ~SDLAxisDirectionToAnyMapping();
    std::string GetPhysicalInputName() override;
    std::string GetPhysicalDeviceName() override;
    bool AxisIsTrigger();
    bool AxisIsStick();

  protected:
    SDL_GameControllerAxis mControllerAxis;
    AxisDirection mAxisDirection;
};
} // namespace Ship
