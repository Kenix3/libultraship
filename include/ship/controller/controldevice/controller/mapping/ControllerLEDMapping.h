#pragma once

#include <cstdint>
#include <string>
#include "ControllerMapping.h"
#include "ship/utils/color.h"

namespace Ship {

#define LED_COLOR_SOURCE_OFF 0
#define LED_COLOR_SOURCE_SET 1
#define LED_COLOR_SOURCE_GAME 2

/**
 * @brief Maps LED colour output to a physical controller.
 *
 * Manages the colour source mode (off, user-set, or game-driven) and the
 * saved colour value. Subclasses implement the device-specific LED colour
 * update for their hardware.
 */
class ControllerLEDMapping : public ControllerMapping {
  public:
    /**
     * @brief Constructs a ControllerLEDMapping.
     * @param physicalDeviceType The type of physical device this mapping targets.
     * @param portIndex          The controller port index this mapping is assigned to.
     * @param colorSource        The LED colour source mode (LED_COLOR_SOURCE_OFF/SET/GAME).
     * @param savedColor         The user-saved LED colour.
     */
    ControllerLEDMapping(PhysicalDeviceType physicalDeviceType, uint8_t portIndex, uint8_t colorSource,
                         Color_RGB8 savedColor);
    ~ControllerLEDMapping();

    /**
     * @brief Sets the LED colour source mode.
     * @param colorSource The colour source constant (LED_COLOR_SOURCE_OFF/SET/GAME).
     */
    void SetColorSource(uint8_t colorSource);

    /**
     * @brief Returns the current LED colour source mode.
     * @return The colour source constant.
     */
    uint8_t GetColorSource();

    /**
     * @brief Applies the given colour to the physical device LED.
     * @param color The RGB colour to set.
     */
    virtual void SetLEDColor(Color_RGB8 color) = 0;

    /**
     * @brief Saves a colour for later restoration.
     * @param colorToSave The RGB colour to persist.
     */
    void SetSavedColor(Color_RGB8 colorToSave);

    /**
     * @brief Returns the previously saved LED colour.
     * @return The saved Color_RGB8 value.
     */
    Color_RGB8 GetSavedColor();

    /**
     * @brief Sets the controller port index for this mapping.
     * @param portIndex The new port index.
     */
    void SetPortIndex(uint8_t portIndex);

    /**
     * @brief Returns a unique string identifier for this LED mapping.
     * @return The mapping identifier string.
     */
    virtual std::string GetLEDMappingId() = 0;

    /** @brief Persists this mapping to the application configuration. */
    virtual void SaveToConfig() = 0;

    /** @brief Removes this mapping from the application configuration. */
    virtual void EraseFromConfig() = 0;

  protected:
    uint8_t mPortIndex;
    uint8_t mColorSource;
    Color_RGB8 mSavedColor;
};
} // namespace Ship
