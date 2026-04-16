#pragma once

#include <string>
#include "ship/controller/physicaldevice/PhysicalDeviceType.h"

namespace Ship {

#define MAPPING_TYPE_UNKNOWN -1
#define MAPPING_TYPE_GAMEPAD 0
#define MAPPING_TYPE_KEYBOARD 1
#define MAPPING_TYPE_MOUSE 2

/**
 * @brief Base class for all controller mappings.
 *
 * ControllerMapping associates a mapping with a physical device type and provides
 * a common interface for querying the device name and type. All specialized mapping
 * classes (button, axis, gyro, rumble, LED) derive from this class.
 */
class ControllerMapping {
  public:
    /**
     * @brief Constructs a ControllerMapping for the given physical device type.
     * @param physicalDeviceType The type of physical device this mapping targets.
     */
    ControllerMapping(PhysicalDeviceType physicalDeviceType);
    ~ControllerMapping();

    /**
     * @brief Returns a human-readable name for the physical device associated with this mapping.
     * @return The physical device name string.
     */
    virtual std::string GetPhysicalDeviceName();

    /**
     * @brief Returns the physical device type for this mapping.
     * @return The PhysicalDeviceType enum value.
     */
    PhysicalDeviceType GetPhysicalDeviceType();

  protected:
    PhysicalDeviceType mPhysicalDeviceType;
};
} // namespace Ship
