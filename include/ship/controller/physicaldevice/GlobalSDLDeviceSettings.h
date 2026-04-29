#pragma once

#include <cstdint>

namespace Ship {

/**
 * @brief Global threshold settings for SDL gamepad analog axes.
 *
 * GlobalSDLDeviceSettings stores stick and trigger dead-zone threshold
 * percentages that apply to all SDL game controllers. The values are
 * persisted through the Config subsystem.
 */
class GlobalSDLDeviceSettings {
  public:
    /** @brief Constructs the settings and loads persisted values from configuration. */
    GlobalSDLDeviceSettings();
    ~GlobalSDLDeviceSettings();

    /**
     * @brief Returns the stick axis dead-zone threshold as a percentage (0–100).
     * @return Current stick axis threshold percentage.
     */
    int32_t GetStickAxisThresholdPercentage();

    /**
     * @brief Sets the stick axis dead-zone threshold percentage.
     * @param stickAxisThresholdPercentage New threshold value (0–100).
     */
    void SetStickAxisThresholdPercentage(int32_t stickAxisThresholdPercentage);

    /**
     * @brief Returns the trigger axis dead-zone threshold as a percentage (0–100).
     * @return Current trigger axis threshold percentage.
     */
    int32_t GetTriggerAxisThresholdPercentage();

    /**
     * @brief Sets the trigger axis dead-zone threshold percentage.
     * @param triggerAxisThresholdPercentage New threshold value (0–100).
     */
    void SetTriggerAxisThresholdPercentage(int32_t triggerAxisThresholdPercentage);

    /** @brief Persists the current threshold values to the Config subsystem. */
    void SaveToConfig();

    /** @brief Removes the persisted threshold values from the Config subsystem. */
    void EraseFromConfig();

  private:
    int32_t mStickAxisThresholdPercentage;
    int32_t mTriggerAxisThresholdPercentage;
};
} // namespace Ship
