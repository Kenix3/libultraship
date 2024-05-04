#pragma once

#include "controller/controldevice/controller/mapping/ControllerLEDMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace Ship {
class LEDMappingFactory {
  public:
    static std::shared_ptr<ControllerLEDMapping> CreateLEDMappingFromConfig(uint8_t portIndex, std::string id);
    static std::shared_ptr<ControllerLEDMapping> CreateLEDMappingFromSDLInput(uint8_t portIndex);
};
} // namespace Ship
