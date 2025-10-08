#include "ship/controller/controldevice/controller/mapping/sdl/SDLLEDMapping.h"

#include "ship/config/ConsoleVariable.h"
#include "ship/utils/StringHelper.h"
#include "ship/Context.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {
SDLLEDMapping::SDLLEDMapping(uint8_t portIndex, uint8_t colorSource, Color_RGB8 savedColor)
    : ControllerLEDMapping(PhysicalDeviceType::SDLGamepad, portIndex, colorSource, savedColor) {
}

void SDLLEDMapping::SetLEDColor(Color_RGB8 color) {
    if (mColorSource == LED_COLOR_SOURCE_OFF) {
        color = { 0, 0, 0 };
    }

    if (mColorSource == LED_COLOR_SOURCE_SET) {
        color = mSavedColor;
    }

    for (const auto& [instanceId, gamepad] :
         Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager()->GetConnectedSDLGamepadsForPort(
             mPortIndex)) {
        if (!SDL_GameControllerHasLED(gamepad)) {
            continue;
        }

        SDL_JoystickSetLED(SDL_GameControllerGetJoystick(gamepad), color.r, color.g, color.b);
    }
}

std::string SDLLEDMapping::GetLEDMappingId() {
    return StringHelper::Sprintf("P%d", mPortIndex);
}

void SDLLEDMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".LEDMappings." + GetLEDMappingId();
    Ship::Context::GetInstance()->GetConsoleVariables()->SetString(
        StringHelper::Sprintf("%s.LEDMappingClass", mappingCvarKey.c_str()).c_str(), "SDLLEDMapping");
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.ColorSource", mappingCvarKey.c_str()).c_str(), mColorSource);
    Ship::Context::GetInstance()->GetConsoleVariables()->SetColor24(
        StringHelper::Sprintf("%s.SavedColor", mappingCvarKey.c_str()).c_str(), mSavedColor);
    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}

void SDLLEDMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".LEDMappings." + GetLEDMappingId();

    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.LEDMappingClass", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.ColorSource", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.SavedColor", mappingCvarKey.c_str()).c_str());

    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}

std::string SDLLEDMapping::GetPhysicalDeviceName() {
    return "SDL Gamepad";
}
} // namespace Ship
