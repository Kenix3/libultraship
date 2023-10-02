#include "SDLGyroMapping.h"
#include "controller/controldevice/controller/mapping/ControllerGyroMapping.h"
#include <spdlog/spdlog.h>

#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>

namespace LUS {
SDLGyroMapping::SDLGyroMapping(uint8_t portIndex, int32_t sdlControllerIndex) : ControllerGyroMapping(portIndex), SDLMapping(sdlControllerIndex) {
}

std::string SDLGyroMapping::GetGyroMappingId() {
    return StringHelper::Sprintf("P%d-SDLI%d", mPortIndex, mControllerIndex);
}

void SDLGyroMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.GyroMappings." + GetGyroMappingId();

    CVarSetString(StringHelper::Sprintf("%s.GyroMappingClass", mappingCvarKey.c_str()).c_str(), "SDLGyroMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), mControllerIndex);
    
    CVarSave();
}

void SDLGyroMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.GyroMappings." + GetGyroMappingId();

    CVarClear(StringHelper::Sprintf("%s.GyroMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str());

    CVarSave();
}

std::string SDLGyroMapping::GetPhysicalDeviceName() {
    return GetSDLDeviceName();
}
} // namespace LUS
