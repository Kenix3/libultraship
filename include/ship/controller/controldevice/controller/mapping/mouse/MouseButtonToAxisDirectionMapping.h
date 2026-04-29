#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "MouseButtonToAnyMapping.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

/**
 * @brief Maps a mouse button to a virtual analog stick direction.
 *
 * When the bound mouse button is held, the mapping reports full deflection
 * in the configured stick direction; when released the value is zero.
 */
class MouseButtonToAxisDirectionMapping final : public MouseButtonToAnyMapping, public ControllerAxisDirectionMapping {
  public:
    /**
     * @brief Constructs a mouse-button-to-axis-direction mapping.
     * @param portIndex  The controller port index.
     * @param stickIndex Which analog stick this mapping targets.
     * @param direction  The stick direction to activate.
     * @param button     The mouse button to bind.
     */
    MouseButtonToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex, Direction direction, MouseBtn button);

    /** @brief Returns the normalised axis value (0 or MAX_AXIS_RANGE). */
    float GetNormalizedAxisDirectionValue() override;

    /** @brief Returns the unique string identifier for this mapping. */
    std::string GetAxisDirectionMappingId() override;

    /** @brief Returns the mapping type identifier. */
    int8_t GetMappingType() override;

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
