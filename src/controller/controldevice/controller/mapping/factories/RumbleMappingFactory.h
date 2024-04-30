#pragma once

#include "controller/controldevice/controller/mapping/ControllerRumbleMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace Ship {
class RumbleMappingFactory {
  public:
    static std::shared_ptr<ControllerRumbleMapping> CreateRumbleMappingFromConfig(uint8_t portIndex, std::string id);

    static std::vector<std::shared_ptr<ControllerRumbleMapping>>
    CreateDefaultSDLRumbleMappings(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex);

    static std::shared_ptr<ControllerRumbleMapping> CreateRumbleMappingFromSDLInput(uint8_t portIndex);
};
} // namespace Ship
