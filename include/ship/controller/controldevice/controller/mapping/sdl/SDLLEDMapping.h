#include "ship/controller/controldevice/controller/mapping/ControllerLEDMapping.h"
#include "SDLMapping.h"

namespace Ship {

/**
 * @brief Maps an SDL gamepad's LED to a controller LED output.
 *
 * Sets the RGB colour of the gamepad LED either from a fixed saved colour or
 * from a dynamic colour source determined at runtime.
 */
class SDLLEDMapping final : public ControllerLEDMapping {
  public:
    /**
     * @brief Constructs an SDL LED mapping.
     * @param portIndex   The controller port index.
     * @param colorSource Identifier for the colour source strategy.
     * @param savedColor  The saved RGB colour used when the source is fixed.
     */
    SDLLEDMapping(uint8_t portIndex, uint8_t colorSource, Color_RGB8 savedColor);

    /**
     * @brief Sends an RGB colour to the gamepad LED.
     * @param color The colour to apply.
     */
    void SetLEDColor(Color_RGB8 color) override;

    /** @brief Returns the unique string identifier for this mapping. */
    std::string GetLEDMappingId() override;

    /** @brief Persists this mapping to the application configuration. */
    void SaveToConfig() override;

    /** @brief Removes this mapping from the application configuration. */
    void EraseFromConfig() override;

    /** @brief Returns the human-readable name of the SDL gamepad device. */
    std::string GetPhysicalDeviceName() override;
};
} // namespace Ship
