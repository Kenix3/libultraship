#ifdef __WIIU__
#include "WiiURumbleMapping.h"

#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>

namespace LUS {
WiiURumbleMapping::WiiURumbleMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex,
                                     uint8_t lowFrequencyIntensityPercentage, uint8_t highFrequencyIntensityPercentage)
    : ControllerRumbleMapping(lusDeviceIndex, portIndex, lowFrequencyIntensityPercentage,
                              highFrequencyIntensityPercentage),
      WiiUMapping(lusDeviceIndex) {
    SetLowFrequencyIntensity(lowFrequencyIntensityPercentage);
    SetHighFrequencyIntensity(highFrequencyIntensityPercentage);
}

void WiiURumbleMapping::StartRumble() {
    if (!ControllerLoaded()) {
        return;
    }

    if (IsGamepad()) {

        return;
    }

    WPADControlMotor(mChan, true);
}

void WiiURumbleMapping::StopRumble() {
    if (!ControllerLoaded()) {
        return;
    }

    if (IsGamepad()) {

        return;
    }

    WPADControlMotor(mChan, false);

    SDL_GameControllerRumble(mController, 0, 0, 0);
}

void WiiURumbleMapping::SetLowFrequencyIntensity(uint8_t intensityPercentage) {
    if (!IsGamepad()) {
        intensityPercentage = 100;
    }
    mLowFrequencyIntensityPercentage = intensityPercentage;
    mHighFrequencyIntensityPercentage = intensityPercentage;
}

void WiiURumbleMapping::SetHighFrequencyIntensity(uint8_t intensityPercentage) {
    if (!IsGamepad()) {
        intensityPercentage = 100;
    }
    mLowFrequencyIntensityPercentage = intensityPercentage;
    mHighFrequencyIntensityPercentage = intensityPercentage;
}

std::string WiiURumbleMapping::GetRumbleMappingId() {
    return StringHelper::Sprintf("P%d-LUSI%d", mPortIndex, ControllerRumbleMapping::mLUSDeviceIndex);
}

void WiiURumbleMapping::SaveToConfig() {
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

void WiiURumbleMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.RumbleMappings." + GetRumbleMappingId();

    CVarClear(StringHelper::Sprintf("%s.RumbleMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LowFrequencyIntensity", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.HighFrequencyIntensity", mappingCvarKey.c_str()).c_str());

    CVarSave();
}

std::string WiiURumbleMapping::GetPhysicalDeviceName() {
    return GetWiiUDeviceName();
}

bool WiiURumbleMapping::PhysicalDeviceIsConnected() {
    return ControllerLoaded();
}
} // namespace LUS
#endif
