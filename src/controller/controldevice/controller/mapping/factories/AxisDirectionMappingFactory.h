#pragma once

#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace LUS {
class AxisDirectionMappingFactory {
  public:
    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromConfig(uint8_t portIndex, Stick stick, std::string id);

#ifdef __WIIU__
    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
    CreateDefaultWiiUAxisDirectionMappings(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, Stick stick);

    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromWiiUInput(uint8_t portIndex, Stick stick, Direction direction);
#else
    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
    CreateDefaultKeyboardAxisDirectionMappings(uint8_t portIndex, Stick stick);

    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
    CreateDefaultSDLAxisDirectionMappings(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, Stick stick);

    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromSDLInput(uint8_t portIndex, Stick stick, Direction direction);
#endif
};
} // namespace LUS
