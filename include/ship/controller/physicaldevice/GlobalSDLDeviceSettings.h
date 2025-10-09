#pragma once

#include <cstdint>

namespace Ship {

class GlobalSDLDeviceSettings {
  public:
    GlobalSDLDeviceSettings();
    ~GlobalSDLDeviceSettings();

    int32_t GetStickAxisThresholdPercentage();
    void SetStickAxisThresholdPercentage(int32_t stickAxisThresholdPercentage);

    int32_t GetTriggerAxisThresholdPercentage();
    void SetTriggerAxisThresholdPercentage(int32_t triggerAxisThresholdPercentage);

    void SaveToConfig();
    void EraseFromConfig();

  private:
    int32_t mStickAxisThresholdPercentage;
    int32_t mTriggerAxisThresholdPercentage;
};
} // namespace Ship
