#pragma once

#include "SDLMapping.h"
#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"

namespace Ship {
class SDLAxisDirectionToAnyMapping : virtual public ControllerInputMapping, public SDLMapping {
  public:
    SDLAxisDirectionToAnyMapping(ShipDeviceIndex shipDeviceIndex, int32_t sdlControllerAxis, int32_t axisDirection);
    ~SDLAxisDirectionToAnyMapping();
    std::string GetPhysicalInputName() override;
    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;
    bool AxisIsTrigger();
    bool AxisIsStick();

  protected:
    SDL_GamepadAxis mControllerAxis;
    AxisDirection mAxisDirection;
};
} // namespace Ship
