#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerInputMapping.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

/**
 * @brief Base class for mappings that bind a mouse scroll-wheel direction to any controller input.
 *
 * Provides shared wheel-direction storage and physical device/input name
 * reporting. Concrete subclasses add the target input semantics (button or
 * axis direction).
 */
class MouseWheelToAnyMapping : virtual public ControllerInputMapping {
  public:
    /**
     * @brief Constructs a mouse-wheel mapping for the given scroll direction.
     * @param wheelDirection The scroll-wheel direction to bind.
     */
    MouseWheelToAnyMapping(WheelDirection wheelDirection);

    /** @brief Destructor. */
    virtual ~MouseWheelToAnyMapping();

    /** @brief Returns the human-readable name of the bound wheel direction. */
    std::string GetPhysicalInputName() override;

    /** @brief Returns the human-readable name of the mouse device. */
    std::string GetPhysicalDeviceName() override;

  protected:
    WheelDirection mWheelDirection;
};
} // namespace Ship
