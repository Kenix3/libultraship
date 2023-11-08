#pragma once

#include <cstdint>
#include <string>
#include "ControllerMapping.h"

namespace LUS {

#define DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE 100
#define DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE 100

class ControllerRumbleMapping : public ControllerMapping {
  public:
    ControllerRumbleMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, uint8_t lowFrequencyIntensityPercentage,
                            uint8_t highFrequencyIntensityPercentage);
    ~ControllerRumbleMapping();
    virtual void StartRumble() = 0;
    virtual void StopRumble() = 0;

    virtual void SetLowFrequencyIntensity(uint8_t intensityPercentage);
    virtual void SetHighFrequencyIntensity(uint8_t intensityPercentage);
    uint8_t GetLowFrequencyIntensityPercentage();
    uint8_t GetHighFrequencyIntensityPercentage();
    bool HighFrequencyIntensityIsDefault();
    bool LowFrequencyIntensityIsDefault();
    void ResetHighFrequencyIntensityToDefault();
    void ResetLowFrequencyIntensityToDefault();
    void SetPortIndex(uint8_t portIndex);

    virtual std::string GetRumbleMappingId() = 0;
    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;

  protected:
    uint8_t mPortIndex;

    uint8_t mLowFrequencyIntensityPercentage;
    uint8_t mHighFrequencyIntensityPercentage;
};
} // namespace LUS
