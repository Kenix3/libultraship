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
    if (IsGamepad()) {
        int32_t patternSize = sizeof(mRumblePattern) * 8;
        VPADControlMotor(VPAD_CHAN_0, mRumblePattern, patternSize);
        return;
    }

    WPADControlMotor(static_cast<WPADChan>(GetWiiUDeviceChannel()), true);
}

void WiiURumbleMapping::StopRumble() {
    if (IsGamepad()) {
        VPADControlMotor(VPAD_CHAN_0, mRumblePattern, 0);
        return;
    }

    WPADControlMotor(static_cast<WPADChan>(GetWiiUDeviceChannel()), false);
}

void WiiURumbleMapping::SetIntensity(uint8_t intensityPercentage) {
    if (!IsGamepad()) {
        intensityPercentage = 100;
        mLowFrequencyIntensityPercentage = 100;
        mHighFrequencyIntensityPercentage = 100;
        return;
    }

    mLowFrequencyIntensityPercentage = intensityPercentage;
    mHighFrequencyIntensityPercentage = intensityPercentage;

    int32_t patternSize = sizeof(mRumblePattern) * 8;
    memset(mRumblePattern, 0, sizeof(mRumblePattern));

    // distribute wanted amount of bits equally in pattern
    float scale = ((intensityPercentage / 100.0f) * (1.0f - 0.3f)) + 0.3f;
    int32_t bitcnt = patternSize * scale;
    for (int32_t i = 0; i < bitcnt; i++) {
        int32_t bitpos = ((i * patternSize) / bitcnt) % patternSize;
        mRumblePattern[bitpos / 8] |= 1 << (bitpos % 8);
    }
}

void WiiURumbleMapping::SetLowFrequencyIntensity(uint8_t intensityPercentage) {
    SetIntensity(intensityPercentage);
}

void WiiURumbleMapping::SetHighFrequencyIntensity(uint8_t intensityPercentage) {
    SetIntensity(intensityPercentage);
}

std::string WiiURumbleMapping::GetRumbleMappingId() {
    return StringHelper::Sprintf("P%d-LUSI%d", mPortIndex, ControllerRumbleMapping::mLUSDeviceIndex);
}

void WiiURumbleMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.RumbleMappings." + GetRumbleMappingId();
    CVarSetString(StringHelper::Sprintf("%s.RumbleMappingClass", mappingCvarKey.c_str()).c_str(), "WiiURumbleMapping");
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
    return WiiUDeviceIsConnected();
}
} // namespace LUS
#endif
