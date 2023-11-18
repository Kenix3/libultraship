#include "ControllerRumbleMapping.h"

namespace LUS {
ControllerRumbleMapping::ControllerRumbleMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex,
                                                 uint8_t lowFrequencyIntensityPercentage,
                                                 uint8_t highFrequencyIntensityPercentage)
    : ControllerMapping(lusDeviceIndex), mPortIndex(portIndex),
      mLowFrequencyIntensityPercentage(lowFrequencyIntensityPercentage),
      mHighFrequencyIntensityPercentage(highFrequencyIntensityPercentage) {
}

ControllerRumbleMapping::~ControllerRumbleMapping() {
}

void ControllerRumbleMapping::SetLowFrequencyIntensity(uint8_t intensityPercentage) {
    mLowFrequencyIntensityPercentage = intensityPercentage;
}

void ControllerRumbleMapping::SetHighFrequencyIntensity(uint8_t intensityPercentage) {
    mHighFrequencyIntensityPercentage = intensityPercentage;
}

void ControllerRumbleMapping::ResetLowFrequencyIntensityToDefault() {
    SetLowFrequencyIntensity(DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE);
}

void ControllerRumbleMapping::ResetHighFrequencyIntensityToDefault() {
    SetHighFrequencyIntensity(DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE);
}

uint8_t ControllerRumbleMapping::GetLowFrequencyIntensityPercentage() {
    return mLowFrequencyIntensityPercentage;
}

uint8_t ControllerRumbleMapping::GetHighFrequencyIntensityPercentage() {
    return mHighFrequencyIntensityPercentage;
}

bool ControllerRumbleMapping::HighFrequencyIntensityIsDefault() {
    return mHighFrequencyIntensityPercentage == DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE;
}

bool ControllerRumbleMapping::LowFrequencyIntensityIsDefault() {
    return mLowFrequencyIntensityPercentage == DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE;
}

void ControllerRumbleMapping::SetPortIndex(uint8_t portIndex) {
    mPortIndex = portIndex;
}
} // namespace LUS
