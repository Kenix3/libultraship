#pragma once

#include "SDLMapping.h"
#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"

namespace LUS {
class SDLAxisDirectionToAnyMapping : virtual public ControllerInputMapping, public SDLMapping {
  public:
    SDLAxisDirectionToAnyMapping(int32_t sdlControllerIndex, int32_t sdlControllerAxis, int32_t axisDirection);
    ~SDLAxisDirectionToAnyMapping();
    std::string GetPhysicalInputName() override;

  protected:
    SDL_GameControllerAxis mControllerAxis;
    AxisDirection mAxisDirection;
};
} // namespace LUS
