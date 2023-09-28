#pragma once

#include "ControllerAxisDirectionMapping.h"
#include <memory>
#include <string>

namespace LUS {
class AxisDirectionMappingFactory {
public:
    static std::shared_ptr<ControllerAxisDirectionMapping> CreateAxisDirectionMappingFromConfig(uint8_t portIndex, Stick stick, std::string id);
};
}
