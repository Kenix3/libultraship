#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace Ship {
class ConsoleVariable;
class ControlDeck;
class Window;

/**
 * @brief Factory for creating ControllerAxisDirectionMapping instances.
 *
 * AxisDirectionMappingFactory provides static helpers to create `ControllerAxisDirectionMapping`
 * instances from various input sources. It supports deserializing axis mappings from
 * persisted configuration, generating default keyboard and SDL mappings, and creating
 * mappings from live SDL input events during interactive binding.
 *
 * All factory methods require a `ConsoleVariable` and `ControlDeck` to track state and
 * configuration. Keyboard and mouse methods also require a `Window` for key-capture state.
 *
 * Typical usage (within ControlDeck or ConfigUI):
 * @code
 * auto mapping = AxisDirectionMappingFactory::CreateAxisDirectionMappingFromConfig(
 *     portIndex, stickIndex, mappingId, consoleVariable, controlDeck, window);
 * @endcode
 */
class AxisDirectionMappingFactory {
  public:
    /**
     * @brief Deserializes an axis-direction mapping from the configuration string.
     * @param portIndex       The controller port index (0-based).
     * @param stickIndex      Which stick (C-Stick or Main Stick).
     * @param id              Configuration string encoding the input source and direction.
     * @param consoleVariable ConsoleVariable for persisting the mapping.
     * @param controlDeck     ControlDeck for physical device access.
     * @param window          Window for keyboard input state tracking.
     * @return The deserialized mapping, or nullptr if the configuration string is invalid.
     */
    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromConfig(uint8_t portIndex, StickIndex stickIndex, std::string id,
                                         std::shared_ptr<ConsoleVariable> consoleVariable,
                                         std::shared_ptr<ControlDeck> controlDeck, std::shared_ptr<Window> window);

    /**
     * @brief Creates default keyboard mappings for the given stick and direction.
     * @param portIndex       The controller port index (0-based).
     * @param stickIndex      Which stick (C-Stick or Main Stick).
     * @param consoleVariable ConsoleVariable for persisting mappings.
     * @param controlDeck     ControlDeck for physical device access.
     * @param window          Window for keyboard input state tracking.
     * @return Vector of keyboard mappings (typically 4, one per direction).
     */
    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> CreateDefaultKeyboardAxisDirectionMappings(
        uint8_t portIndex, StickIndex stickIndex, std::shared_ptr<ConsoleVariable> consoleVariable,
        std::shared_ptr<ControlDeck> controlDeck, std::shared_ptr<Window> window);

    /**
     * @brief Creates default SDL mappings for the given stick.
     * @param portIndex       The controller port index (0-based).
     * @param stickIndex      Which stick (C-Stick or Main Stick).
     * @param consoleVariable ConsoleVariable for persisting mappings.
     * @param controlDeck     ControlDeck for physical device access.
     * @return Vector of SDL axis mappings (typically 4, one per direction).
     */
    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
    CreateDefaultSDLAxisDirectionMappings(uint8_t portIndex, StickIndex stickIndex,
                                          std::shared_ptr<ConsoleVariable> consoleVariable,
                                          std::shared_ptr<ControlDeck> controlDeck);

    /**
     * @brief Creates an axis-direction mapping from live SDL input for interactive binding.
     * @param portIndex       The controller port index (0-based).
     * @param stickIndex      Which stick (C-Stick or Main Stick).
     * @param direction       Direction being mapped (Up, Down, Left, Right).
     * @param consoleVariable ConsoleVariable for persisting the mapping.
     * @param controlDeck     ControlDeck for physical device access.
     * @return The newly created mapping, or nullptr if input detection failed.
     */
    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromSDLInput(uint8_t portIndex, StickIndex stickIndex, Direction direction,
                                           std::shared_ptr<ConsoleVariable> consoleVariable,
                                           std::shared_ptr<ControlDeck> controlDeck);

    /**
     * @brief Creates an axis-direction mapping from mouse wheel input for interactive binding.
     * @param portIndex       The controller port index (0-based).
     * @param stickIndex      Which stick (C-Stick or Main Stick).
     * @param direction       Direction being mapped (Up, Down, Left, Right).
     * @param consoleVariable ConsoleVariable for persisting the mapping.
     * @param controlDeck     ControlDeck for physical device access.
     * @return The newly created mapping, or nullptr if input detection failed.
     */
    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromMouseWheelInput(uint8_t portIndex, StickIndex stickIndex, Direction direction,
                                                  std::shared_ptr<ConsoleVariable> consoleVariable,
                                                  std::shared_ptr<ControlDeck> controlDeck);
};
} // namespace Ship
