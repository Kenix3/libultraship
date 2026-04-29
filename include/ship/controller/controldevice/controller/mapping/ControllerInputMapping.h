#pragma once

#include <string>
#include "ControllerMapping.h"

namespace Ship {

/**
 * @brief Base class for controller mappings that represent a physical input.
 *
 * Extends ControllerMapping to add an interface for querying the name of the
 * specific physical input (e.g. a button label, axis name, or key name).
 * ControllerButtonMapping, ControllerAxisDirectionMapping, and ControllerGyroMapping
 * derive from this class.
 */
class ControllerInputMapping : public ControllerMapping {
  public:
    /**
     * @brief Constructs a ControllerInputMapping for the given physical device type.
     * @param physicalDeviceType The type of physical device this input mapping targets.
     */
    ControllerInputMapping(PhysicalDeviceType physicalDeviceType);
    ~ControllerInputMapping();

    /**
     * @brief Returns a human-readable name for the physical input this mapping represents.
     * @return The physical input name string.
     */
    virtual std::string GetPhysicalInputName();
};
} // namespace Ship
