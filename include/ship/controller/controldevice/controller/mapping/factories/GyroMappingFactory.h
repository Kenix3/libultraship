#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerGyroMapping.h"
#include <memory>
#include <string>

namespace Ship {

/**
 * @brief Factory for creating ControllerGyroMapping instances.
 *
 * Provides static helpers to deserialise gyro mappings from configuration
 * and create mappings from a live SDL gyro sensor.
 */
class GyroMappingFactory {
  public:
    /**
     * @brief Creates a gyro mapping from a saved configuration entry.
     * @param portIndex The controller port index.
     * @param id        The mapping identifier string stored in configuration.
     * @return A shared pointer to the deserialised mapping, or nullptr on failure.
     */
    static std::shared_ptr<ControllerGyroMapping> CreateGyroMappingFromConfig(uint8_t portIndex, std::string id);

    /**
     * @brief Creates a gyro mapping from the currently connected SDL device.
     * @param portIndex The controller port index.
     * @return A shared pointer to the new mapping, or nullptr if no gyro is available.
     */
    static std::shared_ptr<ControllerGyroMapping> CreateGyroMappingFromSDLInput(uint8_t portIndex);
};
} // namespace Ship
