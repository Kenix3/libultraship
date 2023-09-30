#include "ControllerRumbleMapping.h"

namespace LUS {
ControllerRumbleMapping::ControllerRumbleMapping(uint8_t portIndex, uint8_t lowFrequencyIntensityPercentage,
                                                 uint8_t highFrequencyIntensityPercentage)
    : mPortIndex(portIndex), mLowFrequencyIntensityPercentage(lowFrequencyIntensityPercentage),
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

uint8_t ControllerRumbleMapping::GetLowFrequencyIntensityPercentage() {
    return mLowFrequencyIntensityPercentage;
}

uint8_t ControllerRumbleMapping::GetHighFrequencyIntensityPercentage() {
    return mHighFrequencyIntensityPercentage;
}
} // namespace LUS
