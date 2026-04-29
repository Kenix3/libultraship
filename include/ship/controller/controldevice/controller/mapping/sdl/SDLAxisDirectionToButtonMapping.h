#include "ship/controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "ship/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToAnyMapping.h"

namespace Ship {

/**
 * @brief Maps an SDL gamepad axis direction to a virtual controller button.
 *
 * When the bound axis exceeds the activation threshold in the configured
 * direction, the corresponding button bitmask bit is set; otherwise it is cleared.
 */
class SDLAxisDirectionToButtonMapping final : public ControllerButtonMapping, public SDLAxisDirectionToAnyMapping {
  public:
    /**
     * @brief Constructs an SDL axis-direction-to-button mapping.
     * @param portIndex         The controller port index.
     * @param bitmask           The button bitmask to set when the axis is activated.
     * @param sdlControllerAxis The SDL controller axis index.
     * @param axisDirection     The axis half to bind (NEGATIVE or POSITIVE).
     */
    SDLAxisDirectionToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, int32_t sdlControllerAxis,
                                    int32_t axisDirection);

    /**
     * @brief Updates the pad button state based on the current axis value.
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

    /** @brief Returns the human-readable name of the bound axis and direction. */
    std::string GetPhysicalInputName() override;
};
} // namespace Ship
