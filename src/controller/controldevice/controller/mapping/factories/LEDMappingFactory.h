#pragma once

#include "controller/controldevice/controller/mapping/ControllerLEDMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace LUS {
class LEDMappingFactory {
  public:
    static std::shared_ptr<ControllerLEDMapping> CreateLEDMappingFromConfig(uint8_t portIndex, std::string id);

    static std::vector<std::shared_ptr<ControllerLEDMapping>>
    CreateDefaultSDLLEDMappings(uint8_t portIndex, int32_t sdlControllerIndex);

    static std::shared_ptr<ControllerLEDMapping> CreateLEDMappingFromSDLInput(uint8_t portIndex);
};
} // namespace LUS
