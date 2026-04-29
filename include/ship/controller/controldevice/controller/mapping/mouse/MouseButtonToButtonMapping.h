#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "MouseButtonToAnyMapping.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

/**
 * @brief Maps a mouse button to a virtual controller button.
 *
 * When the bound mouse button is held the corresponding button bitmask bit
 * is set in the pad state; when released the bit is cleared.
 */
class MouseButtonToButtonMapping final : public MouseButtonToAnyMapping, public ControllerButtonMapping {
  public:
    /**
     * @brief Constructs a mouse-button-to-button mapping.
     * @param portIndex The controller port index.
     * @param bitmask   The button bitmask to set when the mouse button is held.
     * @param button    The mouse button to bind.
     */
    MouseButtonToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, MouseBtn button);

    /**
     * @brief Updates the pad button state based on the current mouse button state.
     * @param padButtons Reference to the button bitfield to update.
     */
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons) override;

    /** @brief Returns the mapping type identifier. */
    int8_t GetMappingType() override;

    /** @brief Returns the unique string identifier for this mapping. */
    std::string GetButtonMappingId() override;

    /** @brief Persists this mapping to the application configuration. */
    void SaveToConfig() override;

    /** @brief Removes this mapping from the application configuration. */
    void EraseFromConfig() override;

    /** @brief Returns the human-readable name of the mouse device. */
    std::string GetPhysicalDeviceName() override;

    /** @brief Returns the human-readable name of the bound mouse button. */
    std::string GetPhysicalInputName() override;
};
} // namespace Ship
