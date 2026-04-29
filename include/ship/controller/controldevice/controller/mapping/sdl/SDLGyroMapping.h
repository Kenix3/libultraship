#include "ship/controller/controldevice/controller/mapping/ControllerGyroMapping.h"
#include "SDLMapping.h"

namespace Ship {

/**
 * @brief Maps an SDL gamepad's gyroscope sensor to a virtual gyro input.
 *
 * Reads pitch, yaw, and roll from the SDL motion sensor and converts
 * them into X/Y gyro values after subtracting the neutral calibration offsets.
 */
class SDLGyroMapping final : public ControllerGyroMapping {
  public:
    /**
     * @brief Constructs an SDL gyro mapping with calibration offsets.
     * @param portIndex    The controller port index.
     * @param sensitivity  Gyro sensitivity multiplier.
     * @param neutralPitch Pitch value at rest (calibration offset).
     * @param neutralYaw   Yaw value at rest (calibration offset).
     * @param neutralRoll  Roll value at rest (calibration offset).
     */
    SDLGyroMapping(uint8_t portIndex, float sensitivity, float neutralPitch, float neutralYaw, float neutralRoll);

    /**
     * @brief Reads the gyro sensor and writes the resulting X/Y values.
     * @param x Reference to receive the horizontal gyro component.
     * @param y Reference to receive the vertical gyro component.
     */
    void UpdatePad(float& x, float& y) override;

    /** @brief Persists this mapping to the application configuration. */
    void SaveToConfig() override;

    /** @brief Removes this mapping from the application configuration. */
    void EraseFromConfig() override;

    /** @brief Samples the current sensor output and recalculates neutral offsets. */
    void Recalibrate() override;

    /** @brief Returns the unique string identifier for this mapping. */
    std::string GetGyroMappingId() override;

    /** @brief Returns the human-readable name of the SDL gamepad device. */
    std::string GetPhysicalDeviceName() override;

  private:
    float mNeutralPitch;
    float mNeutralYaw;
    float mNeutralRoll;
};
} // namespace Ship
