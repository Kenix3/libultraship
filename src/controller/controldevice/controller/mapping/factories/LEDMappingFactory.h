#pragma once

#include "controller/controldevice/controller/mapping/ControllerLEDMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace LUS {
class LEDMappingFactory {
  public:
    static std::shared_ptr<ControllerLEDMapping> CreateLEDMappingFromConfig(uint8_t portIndex, std::string id);
#ifndef __WIIU__
    static std::shared_ptr<ControllerLEDMapping> CreateLEDMappingFromSDLInput(uint8_t portIndex);
#endif
};
} // namespace LUS
