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
 */
class LEDMappingFactory {
  public:
    static std::shared_ptr<ControllerLEDMapping>
    CreateLEDMappingFromConfig(uint8_t portIndex, std::string id, std::shared_ptr<ConsoleVariable> consoleVariable,
                               std::shared_ptr<ControlDeck> controlDeck);

    static std::shared_ptr<ControllerLEDMapping>
    CreateLEDMappingFromSDLInput(uint8_t portIndex, std::shared_ptr<ConsoleVariable> consoleVariable,
                                 std::shared_ptr<ControlDeck> controlDeck);
};
} // namespace Ship
