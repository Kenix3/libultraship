#pragma once

#include "controller/controldevice/controller/mapping/ControllerRumbleMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace LUS {
class RumbleMappingFactory {
  public:
    static std::shared_ptr<ControllerRumbleMapping> CreateRumbleMappingFromConfig(uint8_t portIndex, std::string id);

    static std::vector<std::shared_ptr<ControllerRumbleMapping>>
    CreateDefaultSDLRumbleMappings(uint8_t portIndex, int32_t sdlControllerIndex);

    static std::shared_ptr<ControllerRumbleMapping> CreateRumbleMappingFromSDLInput(uint8_t portIndex);
};
} // namespace LUS
