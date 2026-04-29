#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace Ship {

/**
 * @brief Factory for creating ControllerAxisDirectionMapping instances.
 *
 * Provides static helpers to deserialise axis-direction mappings from
 * configuration, build default keyboard and SDL mappings, and create
 * mappings from live SDL or mouse-wheel input.
 */
class AxisDirectionMappingFactory {
  public:
    /**
     * @brief Creates an axis-direction mapping from a saved configuration entry.
     * @param portIndex  The controller port index.
     * @param stickIndex Which analog stick the mapping targets.
     * @param id         The mapping identifier string stored in configuration.
     * @return A shared pointer to the deserialised mapping, or nullptr on failure.
     */
    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromConfig(uint8_t portIndex, StickIndex stickIndex, std::string id);

    /**
     * @brief Creates the default set of keyboard axis-direction mappings for a stick.
     * @param portIndex  The controller port index.
     * @param stickIndex Which analog stick to create mappings for.
     * @return A vector of default keyboard axis-direction mappings.
     */
    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
    CreateDefaultKeyboardAxisDirectionMappings(uint8_t portIndex, StickIndex stickIndex);

    /**
     * @brief Creates the default set of SDL axis-direction mappings for a stick.
     * @param portIndex  The controller port index.
     * @param stickIndex Which analog stick to create mappings for.
     * @return A vector of default SDL axis-direction mappings.
     */
    static std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
    CreateDefaultSDLAxisDirectionMappings(uint8_t portIndex, StickIndex stickIndex);

    /**
     * @brief Creates an axis-direction mapping from the next detected SDL input.
     * @param portIndex  The controller port index.
     * @param stickIndex Which analog stick the mapping targets.
     * @param direction  The stick direction to bind.
     * @return A shared pointer to the new mapping, or nullptr if no input was detected.
     */
    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromSDLInput(uint8_t portIndex, StickIndex stickIndex, Direction direction);

    /**
     * @brief Creates an axis-direction mapping from the next detected mouse-wheel input.
     * @param portIndex  The controller port index.
     * @param stickIndex Which analog stick the mapping targets.
     * @param direction  The stick direction to bind.
     * @return A shared pointer to the new mapping, or nullptr if no input was detected.
     */
    static std::shared_ptr<ControllerAxisDirectionMapping>
    CreateAxisDirectionMappingFromMouseWheelInput(uint8_t portIndex, StickIndex stickIndex, Direction direction);
};
} // namespace Ship
