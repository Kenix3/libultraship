#pragma once

#include "ship/controller/controldevice/controller/mapping/sdl/SDLMapping.h"
#include "ship/controller/controldevice/controller/mapping/ControllerInputMapping.h"

namespace Ship {

/**
 * @brief Base class for mappings that bind an SDL gamepad axis direction to any controller input.
 *
 * Provides shared axis-direction storage, physical device/input name reporting,
 * and helpers to determine whether the axis is a trigger or stick.
 */
class SDLAxisDirectionToAnyMapping : virtual public ControllerInputMapping {
  public:
    /**
     * @brief Constructs an SDL axis-direction mapping.
     * @param sdlControllerAxis The SDL controller axis index.
     * @param axisDirection     The axis half to bind (NEGATIVE or POSITIVE).
     */
    SDLAxisDirectionToAnyMapping(int32_t sdlControllerAxis, int32_t axisDirection);

    /** @brief Destructor. */
    virtual ~SDLAxisDirectionToAnyMapping();

    /** @brief Returns the human-readable name of the bound axis and direction. */
    std::string GetPhysicalInputName() override;

    /** @brief Returns the human-readable name of the SDL gamepad device. */
    std::string GetPhysicalDeviceName() override;

    /**
     * @brief Tests whether this axis represents a trigger (e.g. LT / RT).
     * @return true if the axis is a trigger.
     */
    bool AxisIsTrigger();

    /**
     * @brief Tests whether this axis represents an analog stick axis.
     * @return true if the axis is a stick.
     */
    bool AxisIsStick();

  protected:
    SDL_GameControllerAxis mControllerAxis;
    AxisDirection mAxisDirection;
};
} // namespace Ship
