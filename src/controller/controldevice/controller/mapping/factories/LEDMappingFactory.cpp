/*
void SDLLEDMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.LEDMappings." + GetLEDMappingId();
    CVarSetString(StringHelper::Sprintf("%s.LEDMappingClass", mappingCvarKey.c_str()).c_str(), "SDLLEDMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), mControllerIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.ColorSource", mappingCvarKey.c_str()).c_str(), mColorSource);
    CVarSetColor24(StringHelper::Sprintf("%s.SavedColor", mappingCvarKey.c_str()).c_str(), mSavedColor);
    CVarSave();
}
*/

#include "LEDMappingFactory.h"
#include "controller/controldevice/controller/mapping/sdl/SDLLEDMapping.h"
#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>
#include "libultraship/libultra/controller.h"

namespace LUS {

std::vector<std::shared_ptr<ControllerLEDMapping>>
LEDMappingFactory::CreateDefaultSDLLEDMappings(uint8_t portIndex, int32_t sdlControllerIndex) {
    std::vector<std::shared_ptr<ControllerLEDMapping>> mappings;

    mappings.push_back(std::make_shared<SDLLEDMapping>(portIndex, 0, Color_RGB8({0,0,0}), sdlControllerIndex));

    return mappings;
}

std::shared_ptr<ControllerLEDMapping> LEDMappingFactory::CreateLEDMappingFromConfig(uint8_t portIndex,
                                                                                             std::string id) {
    const std::string mappingCvarKey = "gControllers.LEDMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.LEDMappingClass", mappingCvarKey.c_str()).c_str(), "");

    int32_t colorSource =
        CVarGetInteger(StringHelper::Sprintf("%s.ColorSource", mappingCvarKey.c_str()).c_str(), -1);
    Color_RGB8 savedColor =
        CVarGetColor24(StringHelper::Sprintf("%s.SavedColor", mappingCvarKey.c_str()).c_str(), {0, 0, 0});

    if (colorSource != LED_COLOR_SOURCE_OFF && colorSource != LED_COLOR_SOURCE_SET && colorSource != LED_COLOR_SOURCE_GAME) {
        // something about this mapping is invalid
        CVarClear(mappingCvarKey.c_str());
        CVarSave();
        return nullptr;
    }

    if (mappingClass == "SDLLEDMapping") {
        int32_t sdlControllerIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), 0);

        if (sdlControllerIndex < 0) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLLEDMapping>(portIndex, colorSource, savedColor, sdlControllerIndex);
    }

    return nullptr;
}

std::shared_ptr<ControllerLEDMapping> LEDMappingFactory::CreateLEDMappingFromSDLInput(uint8_t portIndex) {
    std::unordered_map<int32_t, SDL_GameController*> sdlControllersWithLEDs;
    std::shared_ptr<ControllerLEDMapping> mapping = nullptr;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            auto controller = SDL_GameControllerOpen(i);
            if (SDL_GameControllerHasLED(controller)) {
                sdlControllersWithLEDs[i] = SDL_GameControllerOpen(i);
            } else {
                SDL_GameControllerClose(controller);
            }
        }
    }

    for (auto [controllerIndex, controller] : sdlControllersWithLEDs) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLLEDMapping>(portIndex, 0, Color_RGB8({0,0,0}), controllerIndex);
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

            mapping = std::make_shared<SDLLEDMapping>(portIndex, 0, Color_RGB8({0, 0, 0}), controllerIndex);
            break;
        }
    }

    for (auto [i, controller] : sdlControllersWithLEDs) {
        SDL_GameControllerClose(controller);
    }

    return mapping;
}
} // namespace LUS