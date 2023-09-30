#include "RumbleMappingFactory.h"
#include "controller/controldevice/controller/mapping/sdl/SDLRumbleMapping.h"
#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>
#include "libultraship/libultra/controller.h"

namespace LUS {

std::vector<std::shared_ptr<ControllerRumbleMapping>>
RumbleMappingFactory::CreateDefaultSDLRumbleMappings(uint8_t portIndex, int32_t sdlControllerIndex) {
    std::vector<std::shared_ptr<ControllerRumbleMapping>> mappings;

    mappings.push_back(std::make_shared<SDLRumbleMapping>(portIndex, 100, 100, sdlControllerIndex));

    return mappings;
}

} // namespace LUS