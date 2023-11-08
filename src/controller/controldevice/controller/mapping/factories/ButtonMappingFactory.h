#pragma once

#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace LUS {
class ButtonMappingFactory {
  public:
    static std::shared_ptr<ControllerButtonMapping> CreateButtonMappingFromConfig(uint8_t portIndex, std::string id);
#ifdef __WIIU__
    static std::vector<std::shared_ptr<ControllerButtonMapping>>
    CreateDefaultWiiUButtonMappings(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, uint16_t bitmask);

    static std::shared_ptr<ControllerButtonMapping> CreateButtonMappingFromWiiUInput(uint8_t portIndex,
                                                                                     uint16_t bitmask);
#else
    static std::vector<std::shared_ptr<ControllerButtonMapping>> CreateDefaultKeyboardButtonMappings(uint8_t portIndex,
                                                                                                     uint16_t bitmask);

    static std::vector<std::shared_ptr<ControllerButtonMapping>>
    CreateDefaultSDLButtonMappings(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, uint16_t bitmask);

    static std::shared_ptr<ControllerButtonMapping> CreateButtonMappingFromSDLInput(uint8_t portIndex,
                                                                                    uint16_t bitmask);
#endif
};
} // namespace LUS
