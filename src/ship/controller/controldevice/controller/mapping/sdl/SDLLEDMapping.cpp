#include "ship/controller/controldevice/controller/mapping/sdl/SDLLEDMapping.h"

#include "ship/config/ConsoleVariable.h"
#include "ship/utils/StringHelper.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {
SDLLEDMapping::SDLLEDMapping(uint8_t portIndex, uint8_t colorSource, Color_RGB8 savedColor,
                             std::shared_ptr<ControlDeck> controlDeck,
                             std::shared_ptr<ConsoleVariable> consoleVariable)
    : ControllerLEDMapping(PhysicalDeviceType::SDLGamepad, portIndex, colorSource, savedColor) {
    mConsoleVariable = std::move(consoleVariable);
    mControlDeck = std::move(controlDeck);
}

void SDLLEDMapping::SetLEDColor(Color_RGB8 color) {
    if (mColorSource == LED_COLOR_SOURCE_OFF) {
        color = { 0, 0, 0 };
    }

    if (mColorSource == LED_COLOR_SOURCE_SET) {
        color = mSavedColor;
    }

    for (const auto& [instanceId, gamepad] :
         mControlDeck->GetConnectedPhysicalDeviceManager()->GetConnectedSDLGamepadsForPort(mPortIndex)) {
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
    mConsoleVariable->SetString(StringHelper::Sprintf("%s.LEDMappingClass", mappingCvarKey.c_str()).c_str(),
                                "SDLLEDMapping");
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.ColorSource", mappingCvarKey.c_str()).c_str(), mColorSource);
    mConsoleVariable->SetColor24(StringHelper::Sprintf("%s.SavedColor", mappingCvarKey.c_str()).c_str(), mSavedColor);
    mConsoleVariable->Save();
}

void SDLLEDMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".LEDMappings." + GetLEDMappingId();

    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.LEDMappingClass", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.ColorSource", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.SavedColor", mappingCvarKey.c_str()).c_str());

    mConsoleVariable->Save();
}

std::string SDLLEDMapping::GetPhysicalDeviceName() {
    return "SDL Gamepad";
}
} // namespace Ship
