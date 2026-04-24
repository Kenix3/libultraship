#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerInputMapping.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

/**
 * @brief Base class for mappings that bind a mouse button to any controller input.
 *
 * Provides shared mouse-button event processing and physical device/input name
 * reporting. Concrete subclasses add the target input semantics (button or
 * axis direction).
 */
class MouseButtonToAnyMapping : virtual public ControllerInputMapping {
  public:
    /**
     * @brief Constructs a mouse-button mapping for the given button.
     * @param button The mouse button to bind.
     */
    MouseButtonToAnyMapping(MouseBtn button);

    /** @brief Destructor. */
    virtual ~MouseButtonToAnyMapping();

    /**
     * @brief Processes a mouse button event and updates the internal pressed state.
     * @param isPressed true if the button was pressed, false if released.
     * @param button    The mouse button associated with the event.
     * @return true if this mapping's button was affected by the event.
     */
    bool ProcessMouseButtonEvent(bool isPressed, MouseBtn button);

    /** @brief Returns the human-readable name of the mouse device. */
    std::string GetPhysicalDeviceName() override;

    /** @brief Returns the human-readable name of the bound mouse button. */
    std::string GetPhysicalInputName() override;

  protected:
    MouseBtn mButton;
    bool mKeyPressed;
};
} // namespace Ship
