#include "ship/controller/controldevice/controller/mapping/ControllerRumbleMapping.h"
#include "SDLMapping.h"

namespace Ship {

/**
 * @brief Maps an SDL gamepad's haptic motors to a controller rumble output.
 *
 * Controls the low-frequency and high-frequency rumble motors through the
 * SDL haptic API, with configurable intensity percentages.
 */
class SDLRumbleMapping final : public ControllerRumbleMapping {
  public:
    /**
     * @brief Constructs an SDL rumble mapping.
     * @param portIndex                        The controller port index.
     * @param lowFrequencyIntensityPercentage   Intensity of the low-frequency motor (0–100).
     * @param highFrequencyIntensityPercentage  Intensity of the high-frequency motor (0–100).
     */
    SDLRumbleMapping(uint8_t portIndex, uint8_t lowFrequencyIntensityPercentage,
                     uint8_t highFrequencyIntensityPercentage);

    /** @brief Starts the rumble effect with the configured intensities. */
    void StartRumble() override;

    /** @brief Stops any active rumble effect. */
    void StopRumble() override;

    /**
     * @brief Sets the low-frequency motor intensity.
     * @param intensityPercentage The intensity percentage (0–100).
     */
    void SetLowFrequencyIntensity(uint8_t intensityPercentage) override;

    /**
     * @brief Sets the high-frequency motor intensity.
     * @param intensityPercentage The intensity percentage (0–100).
     */
    void SetHighFrequencyIntensity(uint8_t intensityPercentage) override;

    /** @brief Returns the unique string identifier for this mapping. */
    std::string GetRumbleMappingId() override;

    /** @brief Persists this mapping to the application configuration. */
    void SaveToConfig() override;

    /** @brief Removes this mapping from the application configuration. */
    void EraseFromConfig() override;

    /** @brief Returns the human-readable name of the SDL gamepad device. */
    std::string GetPhysicalDeviceName() override;

  private:
    uint16_t mLowFrequencyIntensity;
    uint16_t mHighFrequencyIntensity;
};
} // namespace Ship
