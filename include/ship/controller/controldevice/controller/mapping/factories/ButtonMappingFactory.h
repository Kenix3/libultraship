#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include <memory>
#include <string>
#include <vector>

namespace Ship {

/**
 * @brief Factory for creating ControllerButtonMapping instances.
 *
 * Provides static helpers to deserialise button mappings from configuration,
 * build default keyboard and SDL mappings, and create mappings from live SDL
 * or mouse-wheel input.
 */
class ButtonMappingFactory {
  public:
    /**
     * @brief Creates a button mapping from a saved configuration entry.
     * @param portIndex The controller port index.
     * @param id        The mapping identifier string stored in configuration.
     * @return A shared pointer to the deserialised mapping, or nullptr on failure.
     */
    static std::shared_ptr<ControllerButtonMapping> CreateButtonMappingFromConfig(uint8_t portIndex, std::string id);

    /**
     * @brief Creates the default set of keyboard button mappings for a button.
     * @param portIndex The controller port index.
     * @param bitmask   The button bitmask to create mappings for.
     * @return A vector of default keyboard button mappings.
     */
    static std::vector<std::shared_ptr<ControllerButtonMapping>>
    CreateDefaultKeyboardButtonMappings(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask);

    /**
     * @brief Creates the default set of SDL button mappings for a button.
     * @param portIndex The controller port index.
     * @param bitmask   The button bitmask to create mappings for.
     * @return A vector of default SDL button mappings.
     */
    static std::vector<std::shared_ptr<ControllerButtonMapping>>
    CreateDefaultSDLButtonMappings(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask);

    /**
     * @brief Creates a button mapping from the next detected SDL input.
     * @param portIndex The controller port index.
     * @param bitmask   The button bitmask to bind.
     * @return A shared pointer to the new mapping, or nullptr if no input was detected.
     */
    static std::shared_ptr<ControllerButtonMapping> CreateButtonMappingFromSDLInput(uint8_t portIndex,
                                                                                    CONTROLLERBUTTONS_T bitmask);

    /**
     * @brief Creates a button mapping from the next detected mouse-wheel input.
     * @param portIndex The controller port index.
     * @param bitmask   The button bitmask to bind.
     * @return A shared pointer to the new mapping, or nullptr if no input was detected.
     */
    static std::shared_ptr<ControllerButtonMapping> CreateButtonMappingFromMouseWheelInput(uint8_t portIndex,
                                                                                           CONTROLLERBUTTONS_T bitmask);
};
} // namespace Ship
