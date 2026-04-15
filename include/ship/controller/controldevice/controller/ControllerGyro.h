#pragma once

#include <cstdint>
#include <memory>

#include "ship/controller/controldevice/controller/mapping/ControllerGyroMapping.h"

namespace Ship {
/**
 * @brief Manages the single gyroscope (motion sensor) mapping for a controller port.
 *
 * Unlike buttons and sticks, a controller supports at most one ControllerGyroMapping
 * at a time. ControllerGyro provides a thin wrapper that loads/saves the mapping from
 * Config and exposes UpdatePad() to integrate gyroscope data into the game's pad state.
 */
class ControllerGyro {
  public:
    /**
     * @brief Constructs a ControllerGyro for the given port.
     * @param portIndex Zero-based port index.
     */
    ControllerGyro(uint8_t portIndex);
    ~ControllerGyro();

    /** @brief Clears any in-memory mapping and reloads it from Config. */
    void ReloadGyroMappingFromConfig();

    /** @brief Removes the in-memory gyro mapping (does not touch Config). */
    void ClearGyroMapping();

    /** @brief Writes the current gyro mapping ID to Config. */
    void SaveGyroMappingIdToConfig();

    /**
     * @brief Returns the current gyro mapping, or nullptr if none is assigned.
     */
    std::shared_ptr<ControllerGyroMapping> GetGyroMapping();

    /**
     * @brief Sets the active gyro mapping, replacing any previously set one.
     * @param mapping New mapping to use.
     */
    void SetGyroMapping(std::shared_ptr<ControllerGyroMapping> mapping);

    /**
     * @brief Listens for the next raw gyro input and creates a mapping from it.
     * @return true if a mapping was successfully created.
     */
    bool SetGyroMappingFromRawPress();

    /**
     * @brief Reads gyroscope data from the active mapping and writes it to @p x and @p y.
     * @param x Output X-axis (pitch) gyro value.
     * @param y Output Y-axis (yaw/roll) gyro value.
     */
    void UpdatePad(float& x, float& y);

    /**
     * @brief Returns true if the current gyro mapping targets the given device type.
     * @param physicalDeviceType Device type to query.
     */
    bool HasMappingForPhysicalDeviceType(PhysicalDeviceType physicalDeviceType);

  private:
    uint8_t mPortIndex;
    std::shared_ptr<ControllerGyroMapping> mGyroMapping;
};
} // namespace Ship
