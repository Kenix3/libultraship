#include "SDLLEDMapping.h"

#include "public/bridge/consolevariablebridge.h"
#include "utils/StringHelper.h"

namespace Ship {
SDLLEDMapping::SDLLEDMapping(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex, uint8_t colorSource,
                             Color_RGB8 savedColor)
    : ControllerLEDMapping(shipDeviceIndex, portIndex, colorSource, savedColor), SDLMapping(shipDeviceIndex) {
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
    return StringHelper::Sprintf("P%d-SDLI%d", mPortIndex, ControllerLEDMapping::mShipDeviceIndex);
}

void SDLLEDMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".LEDMappings." + GetLEDMappingId();
    CVarSetString(StringHelper::Sprintf("%s.LEDMappingClass", mappingCvarKey.c_str()).c_str(), "SDLLEDMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerLEDMapping::mShipDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.ColorSource", mappingCvarKey.c_str()).c_str(), mColorSource);
    CVarSetColor24(StringHelper::Sprintf("%s.SavedColor", mappingCvarKey.c_str()).c_str(), mSavedColor);
    CVarSave();
}

void SDLLEDMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".LEDMappings." + GetLEDMappingId();

    CVarClear(StringHelper::Sprintf("%s.LEDMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str());
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
} // namespace Ship
