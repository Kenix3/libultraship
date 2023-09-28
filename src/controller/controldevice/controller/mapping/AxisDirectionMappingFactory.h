#pragma once

#include "ControllerAxisDirectionMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace LUS {
class AxisDirectionMappingFactory {
public:
    static std::shared_ptr<ControllerAxisDirectionMapping> CreateAxisDirectionMappingFromConfig(uint8_t portIndex, Stick stick, std::string id);
    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> CreateDefaultKeyboardAxisDirectionMappings(uint8_t portIndex, Stick stick);
    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> CreateDefaultSDLAxisDirectionMappings(uint8_t portIndex, Stick stick, int32_t sdlControllerIndex);
    static std::shared_ptr<ControllerAxisDirectionMapping> CreateAxisDirectionMappingFromRawPress(uint8_t portIndex, Stick stick, Direction direction);
};
}
