#pragma once

#include <cstdint>
#include <string>

#include "ControllerInputMapping.h"

namespace Ship {

#ifndef CONTROLLERBUTTONS_T
#define CONTROLLERBUTTONS_T uint16_t
#endif

/**
 * @brief Maps a physical input to a virtual controller button.
 *
 * Subclasses implement the details for specific device types (keyboard keys,
 * SDL gamepad buttons/axes, etc.) and set the corresponding button bits when
 * the physical input is active.
 */
class ControllerButtonMapping : virtual public ControllerInputMapping {
  public:
    /**
     * @brief Constructs a ControllerButtonMapping.
     * @param physicalDeviceType The type of physical device this mapping targets.
     * @param portIndex          The controller port index this mapping is assigned to.
     * @param bitmask            The button bitmask this mapping controls.
     */
    ControllerButtonMapping(PhysicalDeviceType physicalDeviceType, uint8_t portIndex, CONTROLLERBUTTONS_T bitmask);
    virtual ~ControllerButtonMapping();

    /**
     * @brief Returns a unique string identifier for this button mapping.
     * @return The mapping identifier string.
     */
    virtual std::string GetButtonMappingId() = 0;

    /**
     * @brief Returns the button bitmask this mapping is bound to.
     * @return The button bitmask value.
     */
    CONTROLLERBUTTONS_T GetBitmask();

    /**
     * @brief Reads the physical input state and sets the corresponding bits in padButtons.
     * @param padButtons Reference to the button state bitfield to update.
     */
    virtual void UpdatePad(CONTROLLERBUTTONS_T& padButtons) = 0;

    /**
     * @brief Returns the mapping type identifier (e.g. gamepad, keyboard).
     * @return The mapping type as a MAPPING_TYPE constant.
     */
    virtual int8_t GetMappingType();

    /**
     * @brief Sets the controller port index for this mapping.
     * @param portIndex The new port index.
     */
    void SetPortIndex(uint8_t portIndex);

    /** @brief Persists this mapping to the application configuration. */
    virtual void SaveToConfig() = 0;

    /** @brief Removes this mapping from the application configuration. */
    virtual void EraseFromConfig() = 0;

  protected:
    uint8_t mPortIndex;
    CONTROLLERBUTTONS_T mBitmask;
};
} // namespace Ship
