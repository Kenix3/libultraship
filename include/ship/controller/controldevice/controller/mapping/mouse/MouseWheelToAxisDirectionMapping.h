#pragma once

#include "MouseWheelToAnyMapping.h"
#include "ship/controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

/**
 * @brief Maps a mouse scroll-wheel direction to a virtual analog stick direction.
 *
 * Scrolling in the bound direction reports a deflection value derived from the
 * WheelHandler; when scrolling stops the value decays to zero.
 */
class MouseWheelToAxisDirectionMapping final : public MouseWheelToAnyMapping, public ControllerAxisDirectionMapping {
  public:
    /**
     * @brief Constructs a mouse-wheel-to-axis-direction mapping.
     * @param portIndex      The controller port index.
     * @param stickIndex     Which analog stick this mapping targets.
     * @param direction      The stick direction to activate.
     * @param wheelDirection The scroll-wheel direction to bind.
     */
    MouseWheelToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex, Direction direction,
                                     WheelDirection wheelDirection);

    /** @brief Returns the normalised axis value derived from the wheel input. */
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

    /** @brief Returns the human-readable name of the bound wheel direction. */
    std::string GetPhysicalInputName() override;
};
} // namespace Ship
