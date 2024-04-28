#pragma once

#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace Ship {
class ButtonMappingFactory {
  public:
    static std::shared_ptr<ControllerButtonMapping> CreateButtonMappingFromConfig(uint8_t portIndex, std::string id);
#ifdef __WIIU__
    static std::vector<std::shared_ptr<ControllerButtonMapping>>
    CreateDefaultWiiUButtonMappings(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex, CONTROLLERBUTTONS_T bitmask);

    static std::shared_ptr<ControllerButtonMapping> CreateButtonMappingFromWiiUInput(uint8_t portIndex,
                                                                                     CONTROLLERBUTTONS_T bitmask);
#else
    static std::vector<std::shared_ptr<ControllerButtonMapping>>
    CreateDefaultKeyboardButtonMappings(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask);

    static std::vector<std::shared_ptr<ControllerButtonMapping>>
    CreateDefaultSDLButtonMappings(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex, CONTROLLERBUTTONS_T bitmask);

    static std::shared_ptr<ControllerButtonMapping> CreateButtonMappingFromSDLInput(uint8_t portIndex,
                                                                                    CONTROLLERBUTTONS_T bitmask);
#endif
};
} // namespace Ship
