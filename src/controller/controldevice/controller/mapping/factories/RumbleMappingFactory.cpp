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
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

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

std::shared_ptr<ControllerRumbleMapping> RumbleMappingFactory::CreateRumbleMappingFromSDLInput(uint8_t portIndex) {
    std::unordered_map<int32_t, SDL_GameController*> sdlControllers;
    std::shared_ptr<ControllerRumbleMapping> mapping = nullptr;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            sdlControllers[i] = SDL_GameControllerOpen(i);
        }
    }

    for (auto [controllerIndex, controller] : sdlControllers) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLRumbleMapping>(portIndex, 100, 100, controllerIndex);
                break;
            }
        }

        if (mapping != nullptr) {
            break;
        }

        for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
            const auto axis = static_cast<SDL_GameControllerAxis>(i);
            const auto axisValue = SDL_GameControllerGetAxis(controller, axis) / 32767.0f;
            int32_t axisDirection = 0;
            if (axisValue < -0.7f) {
                axisDirection = NEGATIVE;
            } else if (axisValue > 0.7f) {
                axisDirection = POSITIVE;
            }

            if (axisDirection == 0) {
                continue;
            }

            mapping = std::make_shared<SDLRumbleMapping>(portIndex, 100, 100, controllerIndex);
            break;
        }
    }

    for (auto [i, controller] : sdlControllers) {
        SDL_GameControllerClose(controller);
    }

    return mapping;
}
} // namespace LUS
