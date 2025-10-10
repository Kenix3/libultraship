#include "ship/controller/controldevice/controller/mapping/factories/LEDMappingFactory.h"
#include "ship/controller/controldevice/controller/mapping/sdl/SDLLEDMapping.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/utils/StringHelper.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/Context.h"

namespace Ship {
std::shared_ptr<ControllerLEDMapping> LEDMappingFactory::CreateLEDMappingFromConfig(uint8_t portIndex, std::string id) {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".LEDMappings." + id;
    const std::string mappingClass = Ship::Context::GetInstance()->GetConsoleVariables()->GetString(
        StringHelper::Sprintf("%s.LEDMappingClass", mappingCvarKey.c_str()).c_str(), "");

    int32_t colorSource = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(
        StringHelper::Sprintf("%s.ColorSource", mappingCvarKey.c_str()).c_str(), -1);
    Color_RGB8 savedColor = Ship::Context::GetInstance()->GetConsoleVariables()->GetColor24(
        StringHelper::Sprintf("%s.SavedColor", mappingCvarKey.c_str()).c_str(), { 0, 0, 0 });

    if (colorSource != LED_COLOR_SOURCE_OFF && colorSource != LED_COLOR_SOURCE_SET &&
        colorSource != LED_COLOR_SOURCE_GAME) {
        // something about this mapping is invalid
        Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(mappingCvarKey.c_str());
        Ship::Context::GetInstance()->GetConsoleVariables()->Save();
        return nullptr;
    }

    if (mappingClass == "SDLLEDMapping") {
        return std::make_shared<SDLLEDMapping>(portIndex, colorSource, savedColor);
    }

    return nullptr;
}

std::shared_ptr<ControllerLEDMapping> LEDMappingFactory::CreateLEDMappingFromSDLInput(uint8_t portIndex) {
    std::shared_ptr<ControllerLEDMapping> mapping = nullptr;

    for (auto [instanceId, gamepad] :
         Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager()->GetConnectedSDLGamepadsForPort(
             portIndex)) {
        if (!SDL_GameControllerHasLED(gamepad)) {
            continue;
        }

        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(gamepad, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLLEDMapping>(portIndex, 0, Color_RGB8({ 0, 0, 0 }));
                break;
            }
        }

        if (mapping != nullptr) {
            break;
        }

        for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
            const auto axis = static_cast<SDL_GameControllerAxis>(i);
            const auto axisValue = SDL_GameControllerGetAxis(gamepad, axis) / 32767.0f;
            int32_t axisDirection = 0;
            if (axisValue < -0.7f) {
                axisDirection = NEGATIVE;
            } else if (axisValue > 0.7f) {
                axisDirection = POSITIVE;
            }

            if (axisDirection == 0) {
                continue;
            }

            mapping = std::make_shared<SDLLEDMapping>(portIndex, 0, Color_RGB8({ 0, 0, 0 }));
            break;
        }
    }

    return mapping;
}
} // namespace Ship
