#pragma once

#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace LUS {
class ButtonMappingFactory {
  public:
    static std::shared_ptr<ControllerButtonMapping> CreateButtonMappingFromConfig(uint8_t portIndex, std::string id);
    static std::vector<std::shared_ptr<ControllerButtonMapping>> CreateDefaultKeyboardButtonMappings(uint8_t portIndex,
                                                                                                     uint16_t bitmask);

    static std::vector<std::shared_ptr<ControllerButtonMapping>>
    CreateDefaultSDLButtonMappings(uint8_t portIndex, uint16_t bitmask, int32_t sdlControllerIndex);

    static std::shared_ptr<ControllerButtonMapping> CreateButtonMappingFromSDLInput(uint8_t portIndex,
                                                                                    uint16_t bitmask);
};
} // namespace LUS
