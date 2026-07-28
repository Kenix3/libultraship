#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerGyroMapping.h"
#include <memory>
#include <string>

namespace Ship {
class ConsoleVariable;
class ControlDeck;

/**
 * @brief Factory for creating ControllerGyroMapping instances.
 *
 * GyroMappingFactory provides static helpers to create `ControllerGyroMapping` instances
 * that map gyroscope (motion control) input from SDL controllers to game controller gyro output.
 * It supports deserializing gyro mappings from persisted configuration and creating mappings
 * from live SDL input events during interactive binding.
 *
 * Gyro mappings enable motion-based aim control and other game features that rely on
 * accelerometer/gyroscope input from compatible physical devices.
 *
 * All factory methods require a `ConsoleVariable` and `ControlDeck` to track state and
 * configuration.
 *
 * Typical usage (within ControlDeck or ConfigUI):
 * @code
 * auto mapping = GyroMappingFactory::CreateGyroMappingFromConfig(
 *     portIndex, mappingId, consoleVariable, controlDeck);
 * @endcode
 */
class GyroMappingFactory {
  public:
    /**
     * @brief Deserializes a gyro mapping from the configuration string.
     * @param portIndex       The controller port index (0-based).
     * @param id              Configuration string encoding the gyro input source.
     * @param consoleVariable ConsoleVariable for persisting the mapping.
     * @param controlDeck     ControlDeck for physical device access.
     * @return The deserialized mapping, or nullptr if the configuration string is invalid.
     */
    static std::shared_ptr<ControllerGyroMapping>
    CreateGyroMappingFromConfig(uint8_t portIndex, std::string id, std::shared_ptr<ConsoleVariable> consoleVariable,
                                std::shared_ptr<ControlDeck> controlDeck);

    /**
     * @brief Creates a gyro mapping from live SDL input for interactive binding.
     * @param portIndex       The controller port index (0-based).
     * @param consoleVariable ConsoleVariable for persisting the mapping.
     * @param controlDeck     ControlDeck for physical device access.
     * @return The newly created mapping, or nullptr if no gyro device is detected.
     */
    static std::shared_ptr<ControllerGyroMapping>
    CreateGyroMappingFromSDLInput(uint8_t portIndex, std::shared_ptr<ConsoleVariable> consoleVariable,
                                  std::shared_ptr<ControlDeck> controlDeck);
};
} // namespace Ship
