#include "ship/controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "KeyboardKeyToAnyMapping.h"

namespace Ship {

/**
 * @brief Maps a keyboard key to a virtual controller button.
 *
 * When the bound key is pressed the corresponding button bitmask bit is set
 * in the pad state; when released the bit is cleared.
 */
class KeyboardKeyToButtonMapping final : public KeyboardKeyToAnyMapping, public ControllerButtonMapping {
  public:
    /**
     * @brief Constructs a keyboard-key-to-button mapping.
     * @param portIndex The controller port index.
     * @param bitmask   The button bitmask to set when the key is pressed.
     * @param scancode  The keyboard scan code to bind.
     */
    KeyboardKeyToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, KbScancode scancode);

    /**
     * @brief Updates the pad button state based on the current key state.
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

    /** @brief Returns the human-readable name of the keyboard device. */
    std::string GetPhysicalDeviceName() override;

    /** @brief Returns the human-readable name of the bound key. */
    std::string GetPhysicalInputName() override;
};
} // namespace Ship
