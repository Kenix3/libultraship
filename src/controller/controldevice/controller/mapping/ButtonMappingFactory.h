#pragma once

#include "ControllerButtonMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace LUS {
class ButtonMappingFactory {
public:
    static std::shared_ptr<ControllerButtonMapping> CreateButtonMappingFromConfig(uint8_t portIndex, std::string id);
    static std::vector<std::shared_ptr<ControllerButtonMapping>> CreateDefaultKeyboardButtonMappings(uint8_t portIndex);
    static std::vector<std::shared_ptr<ControllerButtonMapping>> CreateDefaultSDLButtonMappings(uint8_t portIndex, int32_t sdlControllerIndex);
};
}
