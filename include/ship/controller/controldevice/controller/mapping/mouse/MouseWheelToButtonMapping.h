#pragma once

#include "ship/controller/controldevice/controller/mapping/mouse/MouseWheelToAnyMapping.h"
#include "ship/controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

/**
 * @brief Maps a mouse scroll-wheel direction to a virtual controller button.
 *
 * Scrolling in the bound direction sets the corresponding button bitmask bit
 * in the pad state; when scrolling stops the bit is cleared.
 */
class MouseWheelToButtonMapping final : public MouseWheelToAnyMapping, public ControllerButtonMapping {
  public:
    /**
     * @brief Constructs a mouse-wheel-to-button mapping.
     * @param portIndex      The controller port index.
     * @param bitmask        The button bitmask to set when the wheel is scrolled.
     * @param wheelDirection The scroll-wheel direction to bind.
     */
    MouseWheelToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, WheelDirection wheelDirection);

    /**
     * @brief Updates the pad button state based on the current wheel state.
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

    /** @brief Returns the human-readable name of the bound wheel direction. */
    std::string GetPhysicalInputName() override;
};
} // namespace Ship
