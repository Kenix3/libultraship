#include "ship/controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "SDLButtonToAnyMapping.h"

namespace Ship {

/**
 * @brief Maps an SDL gamepad button to a virtual analog stick direction.
 *
 * When the bound gamepad button is held, the mapping reports full deflection
 * in the configured stick direction; when released the value is zero.
 */
class SDLButtonToAxisDirectionMapping final : public ControllerAxisDirectionMapping, public SDLButtonToAnyMapping {
  public:
    /**
     * @brief Constructs an SDL button-to-axis-direction mapping.
     * @param portIndex           The controller port index.
     * @param stickIndex          Which analog stick this mapping targets.
     * @param direction           The stick direction to activate.
     * @param sdlControllerButton The SDL controller button index.
     */
    SDLButtonToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex, Direction direction,
                                    int32_t sdlControllerButton);

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

    /** @brief Returns the human-readable name of the SDL gamepad device. */
    std::string GetPhysicalDeviceName() override;

    /** @brief Returns the human-readable name of the bound button. */
    std::string GetPhysicalInputName() override;
};
} // namespace Ship
