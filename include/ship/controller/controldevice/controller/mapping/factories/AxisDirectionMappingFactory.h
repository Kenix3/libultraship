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
 */
class AxisDirectionMappingFactory {
  public:
    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromConfig(uint8_t portIndex, StickIndex stickIndex, std::string id,
                                         std::shared_ptr<ConsoleVariable> consoleVariable,
                                         std::shared_ptr<ControlDeck> controlDeck, std::shared_ptr<Window> window);

    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> CreateDefaultKeyboardAxisDirectionMappings(
        uint8_t portIndex, StickIndex stickIndex, std::shared_ptr<ConsoleVariable> consoleVariable,
        std::shared_ptr<ControlDeck> controlDeck, std::shared_ptr<Window> window);

    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
    CreateDefaultSDLAxisDirectionMappings(uint8_t portIndex, StickIndex stickIndex,
                                          std::shared_ptr<ConsoleVariable> consoleVariable,
                                          std::shared_ptr<ControlDeck> controlDeck);

    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromSDLInput(uint8_t portIndex, StickIndex stickIndex, Direction direction,
                                           std::shared_ptr<ConsoleVariable> consoleVariable,
                                           std::shared_ptr<ControlDeck> controlDeck);

    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromMouseWheelInput(uint8_t portIndex, StickIndex stickIndex, Direction direction,
                                                  std::shared_ptr<ConsoleVariable> consoleVariable,
                                                  std::shared_ptr<ControlDeck> controlDeck);
};
} // namespace Ship
