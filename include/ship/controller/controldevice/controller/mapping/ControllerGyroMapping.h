#pragma once

#include <cstdint>
#include <string>
#include "ControllerInputMapping.h"

namespace Ship {

#define GYRO_SENSITIVITY_DEFAULT 100

/**
 * @brief Maps a physical gyroscope to virtual gyro input values.
 *
 * Subclasses implement device-specific gyroscope reading and convert raw
 * angular velocity data into X/Y values used by the game. Sensitivity can
 * be adjusted as a percentage of the default value.
 */
class ControllerGyroMapping : virtual public ControllerInputMapping {
  public:
    /**
     * @brief Constructs a ControllerGyroMapping.
     * @param physicalDeviceType The type of physical device this mapping targets.
     * @param portIndex          The controller port index this mapping is assigned to.
     * @param sensitivity        The gyro sensitivity multiplier.
     */
    ControllerGyroMapping(PhysicalDeviceType physicalDeviceType, uint8_t portIndex, float sensitivity);
    virtual ~ControllerGyroMapping();

    /**
     * @brief Reads the physical gyroscope and updates the X and Y gyro values.
     * @param x Reference to the X gyro axis value to update.
     * @param y Reference to the Y gyro axis value to update.
     */
    virtual void UpdatePad(float& x, float& y) = 0;

    /** @brief Persists this mapping to the application configuration. */
    virtual void SaveToConfig() = 0;

    /** @brief Removes this mapping from the application configuration. */
    virtual void EraseFromConfig() = 0;

    /** @brief Recalibrates the gyroscope to establish a new neutral baseline. */
    virtual void Recalibrate() = 0;

    /**
     * @brief Returns a unique string identifier for this gyro mapping.
     * @return The mapping identifier string.
     */
    virtual std::string GetGyroMappingId() = 0;

    /**
     * @brief Returns the current sensitivity multiplier.
     * @return The sensitivity value as a float.
     */
    float GetSensitivity();

    /**
     * @brief Returns the current sensitivity as a percentage of the default.
     * @return The sensitivity percentage (0–255).
     */
    uint8_t GetSensitivityPercent();

    /**
     * @brief Sets the gyro sensitivity from a percentage value.
     * @param sensitivityPercent The new sensitivity percentage.
     */
    void SetSensitivity(uint8_t sensitivityPercent);

    /** @brief Resets the sensitivity to GYRO_SENSITIVITY_DEFAULT. */
    void ResetSensitivityToDefault();

    /**
     * @brief Checks whether the current sensitivity matches the default value.
     * @return true if sensitivity is at the default, false otherwise.
     */
    bool SensitivityIsDefault();

    /**
     * @brief Sets the controller port index for this mapping.
     * @param portIndex The new port index.
     */
    void SetPortIndex(uint8_t portIndex);

  protected:
    uint8_t mPortIndex;
    uint8_t mSensitivityPercent;
    float mSensitivity;
};
} // namespace Ship
