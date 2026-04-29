#include "ship/controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "SDLAxisDirectionToAnyMapping.h"

namespace Ship {

/**
 * @brief Maps an SDL gamepad axis direction to a virtual analog stick direction.
 *
 * Reads the raw SDL axis value and converts it into a normalised deflection
 * for the configured stick and direction.
 */
class SDLAxisDirectionToAxisDirectionMapping final : public ControllerAxisDirectionMapping,
                                                     public SDLAxisDirectionToAnyMapping {
  public:
    /**
     * @brief Constructs an SDL axis-direction-to-axis-direction mapping.
     * @param portIndex          The controller port index.
     * @param stickIndex         Which analog stick this mapping targets.
     * @param direction          The stick direction to activate.
     * @param sdlControllerAxis  The SDL controller axis index.
     * @param axisDirection      The axis half to bind (NEGATIVE or POSITIVE).
     */
    SDLAxisDirectionToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex, Direction direction,
                                           int32_t sdlControllerAxis, int32_t axisDirection);

    /** @brief Returns the normalised axis value from the SDL axis input. */
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

    /** @brief Returns the human-readable name of the bound axis and direction. */
    std::string GetPhysicalInputName() override;
};
} // namespace Ship
