#include "LEDMappingFactory.h"
#ifndef __WIIU__
#include "controller/controldevice/controller/mapping/sdl/SDLLEDMapping.h"
#endif
#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>
#include "libultraship/libultra/controller.h"
#include "Context.h"
#include "controller/deviceindex/LUSDeviceIndexToSDLDeviceIndexMapping.h"

namespace LUS {
std::shared_ptr<ControllerLEDMapping> LEDMappingFactory::CreateLEDMappingFromConfig(uint8_t portIndex, std::string id) {
#ifndef __WIIU__
    const std::string mappingCvarKey = "gControllers.LEDMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.LEDMappingClass", mappingCvarKey.c_str()).c_str(), "");

    int32_t colorSource = CVarGetInteger(StringHelper::Sprintf("%s.ColorSource", mappingCvarKey.c_str()).c_str(), -1);
    Color_RGB8 savedColor =
        CVarGetColor24(StringHelper::Sprintf("%s.SavedColor", mappingCvarKey.c_str()).c_str(), { 0, 0, 0 });

    if (colorSource != LED_COLOR_SOURCE_OFF && colorSource != LED_COLOR_SOURCE_SET &&
        colorSource != LED_COLOR_SOURCE_GAME) {
        // something about this mapping is invalid
        CVarClear(mappingCvarKey.c_str());
        CVarSave();
        return nullptr;
    }

    if (mappingClass == "SDLLEDMapping") {
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        if (lusDeviceIndex < 0) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLLEDMapping>(static_cast<LUSDeviceIndex>(lusDeviceIndex), portIndex, colorSource,
                                               savedColor);
    }
#endif

    return nullptr;
}

#ifndef __WIIU__
std::shared_ptr<ControllerLEDMapping> LEDMappingFactory::CreateLEDMappingFromSDLInput(uint8_t portIndex) {
    std::unordered_map<LUSDeviceIndex, SDL_GameController*> sdlControllersWithLEDs;
    std::shared_ptr<ControllerLEDMapping> mapping = nullptr;
    for (auto [lusIndex, indexMapping] :
         Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->GetAllDeviceIndexMappings()) {
        auto sdlIndexMapping = std::dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(indexMapping);

        if (sdlIndexMapping == nullptr) {
            // this LUS index isn't mapped to an SDL index
            continue;
        }

        auto sdlIndex = sdlIndexMapping->GetSDLDeviceIndex();

        if (!SDL_IsGameController(sdlIndex)) {
            // this SDL device isn't a game controller
            continue;
        }

        auto controller = SDL_GameControllerOpen(sdlIndex);
        if (SDL_GameControllerHasLED(controller)) {
            sdlControllersWithLEDs[lusIndex] = SDL_GameControllerOpen(sdlIndex);
        } else {
            SDL_GameControllerClose(controller);
        }
    }

    for (auto [lusIndex, controller] : sdlControllersWithLEDs) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLLEDMapping>(lusIndex, portIndex, 0, Color_RGB8({ 0, 0, 0 }));
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

            mapping = std::make_shared<SDLLEDMapping>(lusIndex, portIndex, 0, Color_RGB8({ 0, 0, 0 }));
            break;
        }
    }

    for (auto [i, controller] : sdlControllersWithLEDs) {
        SDL_GameControllerClose(controller);
    }

    return mapping;
}
#endif
} // namespace LUS
