#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerLEDMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace Ship {
class ConsoleVariable;
class ControlDeck;

/**
 * @brief Factory for creating ControllerLEDMapping instances.
 *
 * LEDMappingFactory provides static helpers to create `ControllerLEDMapping` instances
 * that map game controller LED output states to physical controller LED devices.
 * It supports deserializing LED mappings from persisted configuration and creating
 * mappings from live SDL input events during interactive binding.
 *
 * LED mappings enable visual feedback (e.g., lighting or color changes) on physical
 * controllers in response to in-game events.
 *
 * All factory methods require a `ConsoleVariable` and `ControlDeck` to track state and
 * configuration.
 *
 * Typical usage (within ControlDeck or ConfigUI):
 * @code
 * auto mapping = LEDMappingFactory::CreateLEDMappingFromConfig(
 *     portIndex, mappingId, consoleVariable, controlDeck);
 * @endcode
 */
class LEDMappingFactory {
  public:
    /**
     * @brief Deserializes an LED mapping from the configuration string.
     * @param portIndex       The controller port index (0-based).
     * @param id              Configuration string encoding the LED output target.
     * @param consoleVariable ConsoleVariable for persisting the mapping.
     * @param controlDeck     ControlDeck for physical device access.
     * @return The deserialized mapping, or nullptr if the configuration string is invalid.
     */
    static std::shared_ptr<ControllerLEDMapping>
    CreateLEDMappingFromConfig(uint8_t portIndex, std::string id, std::shared_ptr<ConsoleVariable> consoleVariable,
                               std::shared_ptr<ControlDeck> controlDeck);

    /**
     * @brief Creates an LED mapping from live SDL input for interactive binding.
     * @param portIndex       The controller port index (0-based).
     * @param consoleVariable ConsoleVariable for persisting the mapping.
     * @param controlDeck     ControlDeck for physical device access.
     * @return The newly created mapping, or nullptr if no LED device is detected.
     */
    static std::shared_ptr<ControllerLEDMapping>
    CreateLEDMappingFromSDLInput(uint8_t portIndex, std::shared_ptr<ConsoleVariable> consoleVariable,
                                 std::shared_ptr<ControlDeck> controlDeck);
};
} // namespace Ship
