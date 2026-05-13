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
 */
class RumbleMappingFactory {
  public:
    static std::shared_ptr<ControllerRumbleMapping>
    CreateRumbleMappingFromConfig(uint8_t portIndex, std::string id, std::shared_ptr<ConsoleVariable> consoleVariable,
                                  std::shared_ptr<ControlDeck> controlDeck);

    static std::vector<std::shared_ptr<ControllerRumbleMapping>>
    CreateDefaultSDLRumbleMappings(PhysicalDeviceType physicalDeviceType, uint8_t portIndex,
                                   std::shared_ptr<ConsoleVariable> consoleVariable,
                                   std::shared_ptr<ControlDeck> controlDeck);

    static std::shared_ptr<ControllerRumbleMapping>
    CreateRumbleMappingFromSDLInput(uint8_t portIndex, std::shared_ptr<ConsoleVariable> consoleVariable,
                                    std::shared_ptr<ControlDeck> controlDeck);
};
} // namespace Ship
