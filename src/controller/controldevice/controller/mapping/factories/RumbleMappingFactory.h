#pragma once

#include "controller/controldevice/controller/mapping/ControllerRumbleMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace LUS {
class RumbleMappingFactory {
  public:
    static std::shared_ptr<ControllerRumbleMapping> CreateRumbleMappingFromConfig(uint8_t portIndex, std::string id);

#ifdef __WIIU__
    static std::vector<std::shared_ptr<ControllerRumbleMapping>>
    CreateDefaultWiiURumbleMappings(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex);
    static std::shared_ptr<ControllerRumbleMapping> CreateRumbleMappingFromWiiUInput(uint8_t portIndex);
#else
    static std::vector<std::shared_ptr<ControllerRumbleMapping>>
    CreateDefaultSDLRumbleMappings(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex);

    static std::shared_ptr<ControllerRumbleMapping> CreateRumbleMappingFromSDLInput(uint8_t portIndex);
#endif
};
} // namespace LUS
