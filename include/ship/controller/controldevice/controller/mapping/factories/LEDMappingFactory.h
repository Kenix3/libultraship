#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerLEDMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace Ship {

/**
 * @brief Factory for creating ControllerLEDMapping instances.
 *
 * Provides static helpers to deserialise LED mappings from configuration
 * and create mappings from a live SDL device with LED support.
 */
class LEDMappingFactory {
  public:
    /**
     * @brief Creates an LED mapping from a saved configuration entry.
     * @param portIndex The controller port index.
     * @param id        The mapping identifier string stored in configuration.
     * @return A shared pointer to the deserialised mapping, or nullptr on failure.
     */
    static std::shared_ptr<ControllerLEDMapping> CreateLEDMappingFromConfig(uint8_t portIndex, std::string id);

    /**
     * @brief Creates an LED mapping from the currently connected SDL device.
     * @param portIndex The controller port index.
     * @return A shared pointer to the new mapping, or nullptr if no LED is available.
     */
    static std::shared_ptr<ControllerLEDMapping> CreateLEDMappingFromSDLInput(uint8_t portIndex);
};
} // namespace Ship
