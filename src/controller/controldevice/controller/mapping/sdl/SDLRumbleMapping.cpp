#include "SDLRumbleMapping.h"

#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>

namespace LUS {
SDLRumbleMapping::SDLRumbleMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex,
                                   uint8_t lowFrequencyIntensityPercentage, uint8_t highFrequencyIntensityPercentage)
    : ControllerRumbleMapping(lusDeviceIndex, portIndex, lowFrequencyIntensityPercentage,
                              highFrequencyIntensityPercentage),
      SDLMapping(lusDeviceIndex) {
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
    return StringHelper::Sprintf("P%d-LUSI%d", mPortIndex, ControllerRumbleMapping::mLUSDeviceIndex);
}

void SDLRumbleMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.RumbleMappings." + GetRumbleMappingId();
    CVarSetString(StringHelper::Sprintf("%s.RumbleMappingClass", mappingCvarKey.c_str()).c_str(), "SDLRumbleMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerRumbleMapping::mLUSDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.LowFrequencyIntensity", mappingCvarKey.c_str()).c_str(),
                   mLowFrequencyIntensityPercentage);
    CVarSetInteger(StringHelper::Sprintf("%s.HighFrequencyIntensity", mappingCvarKey.c_str()).c_str(),
                   mHighFrequencyIntensityPercentage);
    CVarSave();
}

void SDLRumbleMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.RumbleMappings." + GetRumbleMappingId();

    CVarClear(StringHelper::Sprintf("%s.RumbleMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str());
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
} // namespace LUS
