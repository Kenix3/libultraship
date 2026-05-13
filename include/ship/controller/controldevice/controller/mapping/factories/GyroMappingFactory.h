#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerGyroMapping.h"
#include <memory>
#include <string>

namespace Ship {
class ConsoleVariable;
class ControlDeck;

/**
 * @brief Factory for creating ControllerGyroMapping instances.
 */
class GyroMappingFactory {
  public:
    static std::shared_ptr<ControllerGyroMapping>
    CreateGyroMappingFromConfig(uint8_t portIndex, std::string id, std::shared_ptr<ConsoleVariable> consoleVariable,
                                std::shared_ptr<ControlDeck> controlDeck);

    static std::shared_ptr<ControllerGyroMapping>
    CreateGyroMappingFromSDLInput(uint8_t portIndex, std::shared_ptr<ConsoleVariable> consoleVariable,
                                  std::shared_ptr<ControlDeck> controlDeck);
};
} // namespace Ship
