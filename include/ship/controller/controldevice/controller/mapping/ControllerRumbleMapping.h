#pragma once

#include <cstdint>
#include <string>
#include "ControllerMapping.h"

namespace Ship {

#define DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE 50
#define DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE 50

/**
 * @brief Maps rumble (haptic feedback) output to a physical controller.
 *
 * Provides low-frequency and high-frequency rumble intensity control. Subclasses
 * implement the device-specific start/stop rumble behaviour for their hardware.
 */
class ControllerRumbleMapping : public ControllerMapping {
  public:
    /**
     * @brief Constructs a ControllerRumbleMapping.
     * @param physicalDeviceType              The type of physical device this mapping targets.
     * @param portIndex                       The controller port index this mapping is assigned to.
     * @param lowFrequencyIntensityPercentage  Initial low-frequency rumble intensity percentage.
     * @param highFrequencyIntensityPercentage Initial high-frequency rumble intensity percentage.
     */
    ControllerRumbleMapping(PhysicalDeviceType physicalDeviceType, uint8_t portIndex,
                            uint8_t lowFrequencyIntensityPercentage, uint8_t highFrequencyIntensityPercentage);
    ~ControllerRumbleMapping();

    /** @brief Activates rumble on the physical device. */
    virtual void StartRumble() = 0;

    /** @brief Deactivates rumble on the physical device. */
    virtual void StopRumble() = 0;

    /**
     * @brief Sets the low-frequency rumble intensity.
     * @param intensityPercentage The intensity as a percentage (0–100).
     */
    virtual void SetLowFrequencyIntensity(uint8_t intensityPercentage);

    /**
     * @brief Sets the high-frequency rumble intensity.
     * @param intensityPercentage The intensity as a percentage (0–100).
     */
    virtual void SetHighFrequencyIntensity(uint8_t intensityPercentage);

    /**
     * @brief Returns the current low-frequency rumble intensity percentage.
     * @return The intensity percentage.
     */
    uint8_t GetLowFrequencyIntensityPercentage();

    /**
     * @brief Returns the current high-frequency rumble intensity percentage.
     * @return The intensity percentage.
     */
    uint8_t GetHighFrequencyIntensityPercentage();

    /**
     * @brief Checks whether the high-frequency intensity is at its default value.
     * @return true if at default, false otherwise.
     */
    bool HighFrequencyIntensityIsDefault();

    /**
     * @brief Checks whether the low-frequency intensity is at its default value.
     * @return true if at default, false otherwise.
     */
    bool LowFrequencyIntensityIsDefault();

    /** @brief Resets high-frequency rumble intensity to DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE. */
    void ResetHighFrequencyIntensityToDefault();

    /** @brief Resets low-frequency rumble intensity to DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE. */
    void ResetLowFrequencyIntensityToDefault();

    /**
     * @brief Sets the controller port index for this mapping.
     * @param portIndex The new port index.
     */
    void SetPortIndex(uint8_t portIndex);

    /**
     * @brief Returns a unique string identifier for this rumble mapping.
     * @return The mapping identifier string.
     */
    virtual std::string GetRumbleMappingId() = 0;

    /** @brief Persists this mapping to the application configuration. */
    virtual void SaveToConfig() = 0;

    /** @brief Removes this mapping from the application configuration. */
    virtual void EraseFromConfig() = 0;

  protected:
    uint8_t mPortIndex;

    uint8_t mLowFrequencyIntensityPercentage;
    uint8_t mHighFrequencyIntensityPercentage;
};
} // namespace Ship
