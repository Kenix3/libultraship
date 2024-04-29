#pragma once

#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace Ship {
class AxisDirectionMappingFactory {
  public:
    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromConfig(uint8_t portIndex, Stick stick, std::string id);

    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
    CreateDefaultKeyboardAxisDirectionMappings(uint8_t portIndex, Stick stick);

    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
    CreateDefaultSDLAxisDirectionMappings(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex, Stick stick);

    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromSDLInput(uint8_t portIndex, Stick stick, Direction direction);
};
} // namespace Ship
