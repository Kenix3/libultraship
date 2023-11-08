#pragma once

#include "controller/controldevice/controller/mapping/ControllerGyroMapping.h"
#include <memory>
#include <string>

namespace LUS {
class GyroMappingFactory {
  public:
    static std::shared_ptr<ControllerGyroMapping> CreateGyroMappingFromConfig(uint8_t portIndex, std::string id);
#ifdef __WIIU__
    static std::shared_ptr<ControllerGyroMapping> CreateGyroMappingFromWiiUInput(uint8_t portIndex);
#else
    static std::shared_ptr<ControllerGyroMapping> CreateGyroMappingFromSDLInput(uint8_t portIndex);
#endif
};
} // namespace LUS
