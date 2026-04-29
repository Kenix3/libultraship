#include "ship/controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "ship/controller/controldevice/controller/mapping/sdl/SDLButtonToAnyMapping.h"

namespace Ship {

/**
 * @brief Maps an SDL gamepad button to a virtual controller button.
 *
 * When the bound gamepad button is held the corresponding button bitmask bit
 * is set in the pad state; when released the bit is cleared.
 */
class SDLButtonToButtonMapping final : public SDLButtonToAnyMapping, public ControllerButtonMapping {
  public:
    /**
     * @brief Constructs an SDL button-to-button mapping.
     * @param portIndex           The controller port index.
     * @param bitmask             The button bitmask to set when the gamepad button is held.
     * @param sdlControllerButton The SDL controller button index.
     */
    SDLButtonToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, int32_t sdlControllerButton);

    /**
     * @brief Updates the pad button state based on the current gamepad button state.
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

    /** @brief Returns the human-readable name of the SDL gamepad device. */
    std::string GetPhysicalDeviceName() override;

    /** @brief Returns the human-readable name of the bound button. */
    std::string GetPhysicalInputName() override;
};
} // namespace Ship
