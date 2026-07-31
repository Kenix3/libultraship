#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerRumbleMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace Ship {
class ConsoleVariable;
class ControlDeck;

/**
 * @brief Factory for creating ControllerRumbleMapping instances.
 *
 * RumbleMappingFactory provides static helpers to create `ControllerRumbleMapping` instances
 * that map game controller rumble/haptic output to physical device rumble motors.
 * It supports deserializing rumble mappings from persisted configuration, generating default
 * SDL rumble mappings for known device types, and creating mappings from live SDL input
 * events during interactive binding.
 *
 * Rumble mappings enable force feedback (vibration) on physical controllers in response to
 * in-game events, enhancing immersion and providing tactile feedback.
 *
 * All factory methods require a `ConsoleVariable` and `ControlDeck` to track state and
 * configuration.
 *
 * Typical usage (within ControlDeck or ConfigUI):
 * @code
 * auto mapping = RumbleMappingFactory::CreateRumbleMappingFromConfig(
 *     portIndex, mappingId, consoleVariable, controlDeck);
 * @endcode
 */
class RumbleMappingFactory {
  public:
    /**
     * @brief Deserializes a rumble mapping from the configuration string.
     * @param portIndex       The controller port index (0-based).
     * @param id              Configuration string encoding the rumble output target.
     * @param consoleVariable ConsoleVariable for persisting the mapping.
     * @param controlDeck     ControlDeck for physical device access.
     * @return The deserialized mapping, or nullptr if the configuration string is invalid.
     */
    static std::shared_ptr<ControllerRumbleMapping>
    CreateRumbleMappingFromConfig(uint8_t portIndex, std::string id, std::shared_ptr<ConsoleVariable> consoleVariable,
                                  std::shared_ptr<ControlDeck> controlDeck);

    /**
     * @brief Creates default SDL rumble mappings for a specific physical device type.
     * @param physicalDeviceType SDL gamepad type.
     * @param portIndex          Controller port index (0-based).
     * @param consoleVariable    ConsoleVariable for persisting mappings.
     * @param controlDeck        ControlDeck for physical device access.
     * @return Vector of rumble mappings suitable for the device type.
     */
    static std::vector<std::shared_ptr<ControllerRumbleMapping>>
    CreateDefaultSDLRumbleMappings(PhysicalDeviceType physicalDeviceType, uint8_t portIndex,
                                   std::shared_ptr<ConsoleVariable> consoleVariable,
                                   std::shared_ptr<ControlDeck> controlDeck);

    /**
     * @brief Creates a rumble mapping from live SDL input for interactive binding.
     * @param portIndex       The controller port index (0-based).
     * @param consoleVariable ConsoleVariable for persisting the mapping.
     * @param controlDeck     ControlDeck for physical device access.
     * @return The newly created mapping, or nullptr if no rumble device is detected.
     */
    static std::shared_ptr<ControllerRumbleMapping>
    CreateRumbleMappingFromSDLInput(uint8_t portIndex, std::shared_ptr<ConsoleVariable> consoleVariable,
                                    std::shared_ptr<ControlDeck> controlDeck);
};
} // namespace Ship
