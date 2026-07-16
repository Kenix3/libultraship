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
 * Provides static helpers to deserialise button mappings from configuration,
 * build default keyboard and SDL mappings, and create mappings from live SDL
 * or mouse-wheel input.
 */
class ButtonMappingFactory {
  public:
    static std::shared_ptr<ControllerButtonMapping>
    CreateButtonMappingFromConfig(uint8_t portIndex, std::string id, std::shared_ptr<ConsoleVariable> consoleVariable,
                                  std::shared_ptr<ControlDeck> controlDeck, std::shared_ptr<Window> window);

    static std::vector<std::shared_ptr<ControllerButtonMapping>>
    CreateDefaultKeyboardButtonMappings(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                        std::shared_ptr<ConsoleVariable> consoleVariable,
                                        std::shared_ptr<ControlDeck> controlDeck, std::shared_ptr<Window> window);

    static std::vector<std::shared_ptr<ControllerButtonMapping>>
    CreateDefaultSDLButtonMappings(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                   std::shared_ptr<ConsoleVariable> consoleVariable,
                                   std::shared_ptr<ControlDeck> controlDeck);

    static std::shared_ptr<ControllerButtonMapping>
    CreateButtonMappingFromSDLInput(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                    std::shared_ptr<ConsoleVariable> consoleVariable,
                                    std::shared_ptr<ControlDeck> controlDeck);

    static std::shared_ptr<ControllerButtonMapping>
    CreateButtonMappingFromMouseWheelInput(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                           std::shared_ptr<ConsoleVariable> consoleVariable,
                                           std::shared_ptr<ControlDeck> controlDeck);
};
} // namespace Ship
