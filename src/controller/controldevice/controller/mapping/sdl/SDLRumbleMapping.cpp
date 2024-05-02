#include "SDLRumbleMapping.h"

#include "public/bridge/consolevariablebridge.h"
#include "utils/StringHelper.h"

namespace Ship {
SDLRumbleMapping::SDLRumbleMapping(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex,
                                   uint8_t lowFrequencyIntensityPercentage, uint8_t highFrequencyIntensityPercentage)
    : ControllerRumbleMapping(shipDeviceIndex, portIndex, lowFrequencyIntensityPercentage,
                              highFrequencyIntensityPercentage),
      SDLMapping(shipDeviceIndex) {
    SetLowFrequencyIntensity(lowFrequencyIntensityPercentage);
    SetHighFrequencyIntensity(highFrequencyIntensityPercentage);
}

void SDLRumbleMapping::StartRumble() {
    if (!ControllerLoaded()) {
        return;
    }

    SDL_GameControllerRumble(mController, mLowFrequencyIntensity, mHighFrequencyIntensity, 0);
}

void SDLRumbleMapping::StopRumble() {
    if (!ControllerLoaded()) {
        return;
    }

    SDL_GameControllerRumble(mController, 0, 0, 0);
}

void SDLRumbleMapping::SetLowFrequencyIntensity(uint8_t intensityPercentage) {
    mLowFrequencyIntensityPercentage = intensityPercentage;
    mLowFrequencyIntensity = UINT16_MAX * (intensityPercentage / 100.0f);
}

void SDLRumbleMapping::SetHighFrequencyIntensity(uint8_t intensityPercentage) {
    mHighFrequencyIntensityPercentage = intensityPercentage;
    mHighFrequencyIntensity = UINT16_MAX * (intensityPercentage / 100.0f);
}

std::string SDLRumbleMapping::GetRumbleMappingId() {
    return StringHelper::Sprintf("P%d-LUSI%d", mPortIndex, ControllerRumbleMapping::mShipDeviceIndex);
}

void SDLRumbleMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".RumbleMappings." + GetRumbleMappingId();
    CVarSetString(StringHelper::Sprintf("%s.RumbleMappingClass", mappingCvarKey.c_str()).c_str(), "SDLRumbleMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerRumbleMapping::mShipDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.LowFrequencyIntensity", mappingCvarKey.c_str()).c_str(),
                   mLowFrequencyIntensityPercentage);
    CVarSetInteger(StringHelper::Sprintf("%s.HighFrequencyIntensity", mappingCvarKey.c_str()).c_str(),
                   mHighFrequencyIntensityPercentage);
    CVarSave();
}

void SDLRumbleMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".RumbleMappings." + GetRumbleMappingId();

    CVarClear(StringHelper::Sprintf("%s.RumbleMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LowFrequencyIntensity", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.HighFrequencyIntensity", mappingCvarKey.c_str()).c_str());

    CVarSave();
}

std::string SDLRumbleMapping::GetPhysicalDeviceName() {
    return GetSDLDeviceName();
}

bool SDLRumbleMapping::PhysicalDeviceIsConnected() {
    return ControllerLoaded();
}
} // namespace Ship
