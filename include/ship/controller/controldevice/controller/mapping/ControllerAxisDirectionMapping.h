#pragma once

#include <cstdint>
#include <string>

#include "ControllerInputMapping.h"

#define MAX_AXIS_RANGE 85.0f

namespace Ship {

/** @brief Identifies which analog stick an axis mapping applies to. */
enum StickIndex { LEFT_STICK, RIGHT_STICK };

/** @brief Identifies the direction component of an axis mapping. */
enum Direction { LEFT, RIGHT, UP, DOWN };

/**
 * @brief Maps a physical input to a virtual analog stick direction.
 *
 * Subclasses implement the details for specific device types (keyboard keys,
 * SDL gamepad axes/buttons, etc.) and convert raw input into a normalised
 * axis value in the range [0, MAX_AXIS_RANGE].
 */
class ControllerAxisDirectionMapping : virtual public ControllerInputMapping {
  public:
    /**
     * @brief Constructs a ControllerAxisDirectionMapping.
     * @param physicalDeviceType The type of physical device this mapping targets.
     * @param portIndex          The controller port index this mapping is assigned to.
     * @param stickIndex         Which analog stick this mapping affects.
     * @param direction          The direction on the stick this mapping represents.
     */
    ControllerAxisDirectionMapping(PhysicalDeviceType physicalDeviceType, uint8_t portIndex, StickIndex stickIndex,
                                   Direction direction);
    virtual ~ControllerAxisDirectionMapping();

    /**
     * @brief Returns the current axis value normalised to [0, MAX_AXIS_RANGE].
     * @return The normalised axis direction value.
     */
    virtual float GetNormalizedAxisDirectionValue() = 0;

    /**
     * @brief Returns the mapping type identifier (e.g. gamepad, keyboard).
     * @return The mapping type as a MAPPING_TYPE constant.
     */
    virtual int8_t GetMappingType();

    /**
     * @brief Returns a unique string identifier for this axis direction mapping.
     * @return The mapping identifier string.
     */
    virtual std::string GetAxisDirectionMappingId() = 0;

    /** @brief Persists this mapping to the application configuration. */
    virtual void SaveToConfig() = 0;

    /** @brief Removes this mapping from the application configuration. */
    virtual void EraseFromConfig() = 0;

    /**
     * @brief Returns the direction this mapping represents.
     * @return The Direction enum value.
     */
    Direction GetDirection();

    /**
     * @brief Sets the controller port index for this mapping.
     * @param portIndex The new port index.
     */
    void SetPortIndex(uint8_t portIndex);

  protected:
    uint8_t mPortIndex;
    StickIndex mStickIndex;
    Direction mDirection;
};
} // namespace Ship
