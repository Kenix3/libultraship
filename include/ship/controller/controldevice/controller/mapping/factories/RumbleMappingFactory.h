#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerRumbleMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace Ship {

/**
 * @brief Factory for creating ControllerRumbleMapping instances.
 *
 * Provides static helpers to deserialise rumble mappings from configuration,
 * build default SDL rumble mappings, and create mappings from a live SDL device.
 */
class RumbleMappingFactory {
  public:
    /**
     * @brief Creates a rumble mapping from a saved configuration entry.
     * @param portIndex The controller port index.
     * @param id        The mapping identifier string stored in configuration.
     * @return A shared pointer to the deserialised mapping, or nullptr on failure.
     */
    static std::shared_ptr<ControllerRumbleMapping> CreateRumbleMappingFromConfig(uint8_t portIndex, std::string id);

    /**
     * @brief Creates the default set of SDL rumble mappings for a device type.
     * @param physicalDeviceType The type of physical device to create defaults for.
     * @param portIndex          The controller port index.
     * @return A vector of default SDL rumble mappings.
     */
    static std::vector<std::shared_ptr<ControllerRumbleMapping>>
    CreateDefaultSDLRumbleMappings(PhysicalDeviceType physicalDeviceType, uint8_t portIndex);

    /**
     * @brief Creates a rumble mapping from the currently connected SDL device.
     * @param portIndex The controller port index.
     * @return A shared pointer to the new mapping, or nullptr if no rumble is available.
     */
    static std::shared_ptr<ControllerRumbleMapping> CreateRumbleMappingFromSDLInput(uint8_t portIndex);
};
} // namespace Ship
