#pragma once

#include "ship/controller/controldevice/controller/mapping/sdl/SDLMapping.h"
#include "ship/controller/controldevice/controller/mapping/ControllerInputMapping.h"

namespace Ship {

/**
 * @brief Base class for mappings that bind an SDL gamepad button to any controller input.
 *
 * Provides shared button storage, physical device/input name reporting, and a
 * helper for generating generic button labels when the SDL database lacks a
 * human-readable name.
 */
class SDLButtonToAnyMapping : virtual public ControllerInputMapping {
  public:
    /**
     * @brief Constructs an SDL button mapping.
     * @param sdlControllerButton The SDL controller button index.
     */
    SDLButtonToAnyMapping(int32_t sdlControllerButton);

    /** @brief Destructor. */
    virtual ~SDLButtonToAnyMapping();

    /** @brief Returns the human-readable name of the bound button. */
    std::string GetPhysicalInputName() override;

    /** @brief Returns the human-readable name of the SDL gamepad device. */
    std::string GetPhysicalDeviceName() override;

  protected:
    SDL_GameControllerButton mControllerButton;

  private:
    std::string GetGenericButtonName();
};
} // namespace Ship
