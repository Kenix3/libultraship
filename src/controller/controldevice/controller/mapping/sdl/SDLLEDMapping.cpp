#include "SDLLEDMapping.h"

#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>

namespace LUS {
SDLLEDMapping::SDLLEDMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, uint8_t colorSource,
                             Color_RGB8 savedColor)
    : ControllerLEDMapping(lusDeviceIndex, portIndex, colorSource, savedColor), SDLMapping(lusDeviceIndex) {
}

void SDLLEDMapping::SetLEDColor(Color_RGB8 color) {
    if (!ControllerLoaded()) {
        return;
    }

    if (!SDL_GameControllerHasLED(mController)) {
        return;
    }

    if (mColorSource == LED_COLOR_SOURCE_OFF) {
        color = { 0, 0, 0 };
    }

    if (mColorSource == LED_COLOR_SOURCE_SET) {
        color = mSavedColor;
    }

    SDL_JoystickSetLED(SDL_GameControllerGetJoystick(mController), color.r, color.g, color.b);
}

std::string SDLLEDMapping::GetLEDMappingId() {
    return StringHelper::Sprintf("P%d-SDLI%d", mPortIndex, ControllerLEDMapping::mLUSDeviceIndex);
}

void SDLLEDMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.LEDMappings." + GetLEDMappingId();
    CVarSetString(StringHelper::Sprintf("%s.LEDMappingClass", mappingCvarKey.c_str()).c_str(), "SDLLEDMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerLEDMapping::mLUSDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.ColorSource", mappingCvarKey.c_str()).c_str(), mColorSource);
    CVarSetColor24(StringHelper::Sprintf("%s.SavedColor", mappingCvarKey.c_str()).c_str(), mSavedColor);
    CVarSave();
}

void SDLLEDMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.LEDMappings." + GetLEDMappingId();

    CVarClear(StringHelper::Sprintf("%s.LEDMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.ColorSource", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SavedColor", mappingCvarKey.c_str()).c_str());

    CVarSave();
}

std::string SDLLEDMapping::GetPhysicalDeviceName() {
    return GetSDLDeviceName();
}

bool SDLLEDMapping::PhysicalDeviceIsConnected() {
    return ControllerLoaded();
}
} // namespace LUS
