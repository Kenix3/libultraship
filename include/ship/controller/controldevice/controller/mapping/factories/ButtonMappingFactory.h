#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace Ship {
class ConsoleVariable;
class ControlDeck;
class Window;

/**
 * @brief Factory for creating ControllerButtonMapping instances.
 *
 * ButtonMappingFactory provides static helpers to create `ControllerButtonMapping` instances
 * that map physical input sources (keyboard, mouse, SDL gamepad) to game controller buttons.
 * It supports deserializing button mappings from persisted configuration, generating default
 * keyboard and SDL mappings, and creating mappings from live SDL or mouse-wheel input
 * during interactive binding.
 *
 * All factory methods require a `ConsoleVariable` and `ControlDeck` to track state and
 * configuration. Keyboard methods also require a `Window` for key-capture state.
 *
 * Typical usage (within ControlDeck or ConfigUI):
 * @code
 * auto mapping = ButtonMappingFactory::CreateButtonMappingFromConfig(
 *     portIndex, mappingId, consoleVariable, controlDeck, window);
 * @endcode
 */
class ButtonMappingFactory {
  public:
    /**
     * @brief Deserializes a button mapping from the configuration string.
     * @param portIndex       The controller port index (0-based).
     * @param id              Configuration string encoding the input source and button.
     * @param consoleVariable ConsoleVariable for persisting the mapping.
     * @param controlDeck     ControlDeck for physical device access.
     * @param window          Window for keyboard input state tracking.
     * @return The deserialized mapping, or nullptr if the configuration string is invalid.
     */
    static std::shared_ptr<ControllerButtonMapping>
    CreateButtonMappingFromConfig(uint8_t portIndex, std::string id, std::shared_ptr<ConsoleVariable> consoleVariable,
                                  std::shared_ptr<ControlDeck> controlDeck, std::shared_ptr<Window> window);

    /**
     * @brief Creates default keyboard mappings for the given button bitmask.
     * @param portIndex       The controller port index (0-based).
     * @param bitmask         Bitmask of buttons to create mappings for.
     * @param consoleVariable ConsoleVariable for persisting mappings.
     * @param controlDeck     ControlDeck for physical device access.
     * @param window          Window for keyboard input state tracking.
     * @return Vector of keyboard button mappings.
     */
    static std::vector<std::shared_ptr<ControllerButtonMapping>>
    CreateDefaultKeyboardButtonMappings(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                        std::shared_ptr<ConsoleVariable> consoleVariable,
                                        std::shared_ptr<ControlDeck> controlDeck, std::shared_ptr<Window> window);

    /**
     * @brief Creates default SDL mappings for the given button bitmask.
     * @param portIndex       The controller port index (0-based).
     * @param bitmask         Bitmask of buttons to create mappings for.
     * @param consoleVariable ConsoleVariable for persisting mappings.
     * @param controlDeck     ControlDeck for physical device access.
     * @return Vector of SDL button mappings.
     */
    static std::vector<std::shared_ptr<ControllerButtonMapping>>
    CreateDefaultSDLButtonMappings(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                   std::shared_ptr<ConsoleVariable> consoleVariable,
                                   std::shared_ptr<ControlDeck> controlDeck);

    /**
     * @brief Creates a button mapping from live SDL input for interactive binding.
     * @param portIndex       The controller port index (0-based).
     * @param bitmask         Bitmask of buttons being mapped.
     * @param consoleVariable ConsoleVariable for persisting the mapping.
     * @param controlDeck     ControlDeck for physical device access.
     * @return The newly created mapping, or nullptr if input detection failed.
     */
    static std::shared_ptr<ControllerButtonMapping>
    CreateButtonMappingFromSDLInput(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                    std::shared_ptr<ConsoleVariable> consoleVariable,
                                    std::shared_ptr<ControlDeck> controlDeck);

    /**
     * @brief Creates a button mapping from mouse wheel input for interactive binding.
     * @param portIndex       The controller port index (0-based).
     * @param bitmask         Bitmask of buttons being mapped.
     * @param consoleVariable ConsoleVariable for persisting the mapping.
     * @param controlDeck     ControlDeck for physical device access.
     * @return The newly created mapping, or nullptr if input detection failed.
     */
    static std::shared_ptr<ControllerButtonMapping>
    CreateButtonMappingFromMouseWheelInput(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                           std::shared_ptr<ConsoleVariable> consoleVariable,
                                           std::shared_ptr<ControlDeck> controlDeck);
};
} // namespace Ship
