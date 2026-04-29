#include "ship/controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "KeyboardKeyToAnyMapping.h"

namespace Ship {

/**
 * @brief Maps a keyboard key to a virtual analog stick direction.
 *
 * When the bound key is pressed the mapping reports full deflection in the
 * configured direction; when released the value is zero.
 */
class KeyboardKeyToAxisDirectionMapping final : public KeyboardKeyToAnyMapping, public ControllerAxisDirectionMapping {
  public:
    /**
     * @brief Constructs a keyboard-key-to-axis-direction mapping.
     * @param portIndex  The controller port index.
     * @param stickIndex Which analog stick this mapping targets.
     * @param direction  The stick direction to activate.
     * @param scancode   The keyboard scan code to bind.
     */
    KeyboardKeyToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex, Direction direction,
                                      KbScancode scancode);

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

    /** @brief Returns the human-readable name of the keyboard device. */
    std::string GetPhysicalDeviceName() override;

    /** @brief Returns the human-readable name of the bound key. */
    std::string GetPhysicalInputName() override;
};
} // namespace Ship
