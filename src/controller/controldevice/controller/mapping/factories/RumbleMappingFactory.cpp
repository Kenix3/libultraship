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

std::shared_ptr<ControllerRumbleMapping> RumbleMappingFactory::CreateRumbleMappingFromConfig(uint8_t portIndex,
                                                                                             std::string id) {
    const std::string mappingCvarKey = "gControllers.RumbleMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.RumbleMappingClass", mappingCvarKey.c_str()).c_str(), "");

    int32_t lowFrequencyIntensityPercentage =
        CVarGetInteger(StringHelper::Sprintf("%s.LowFrequencyIntensity", mappingCvarKey.c_str()).c_str(), -1);
    int32_t highFrequencyIntensityPercentage =
        CVarGetInteger(StringHelper::Sprintf("%s.HighFrequencyIntensity", mappingCvarKey.c_str()).c_str(), -1);

    if (lowFrequencyIntensityPercentage < 0 || lowFrequencyIntensityPercentage > 100 ||
        highFrequencyIntensityPercentage < 0 || highFrequencyIntensityPercentage > 100) {
        // something about this mapping is invalid
        CVarClear(mappingCvarKey.c_str());
        CVarSave();
        return nullptr;
    }

    if (mappingClass == "SDLRumbleMapping") {
        int32_t sdlControllerIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), 0);

        if (sdlControllerIndex < 0) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLRumbleMapping>(portIndex, lowFrequencyIntensityPercentage,
                                                  highFrequencyIntensityPercentage, sdlControllerIndex);
    }

    return nullptr;
}

} // namespace LUS