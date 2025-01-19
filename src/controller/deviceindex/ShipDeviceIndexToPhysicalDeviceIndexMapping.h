#pragma once

#include <cstdint>
#include <string>
#include "controller/physicaldevice/PhysicalDeviceType.h"

namespace Ship {

class ShipDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    ShipDeviceIndexToPhysicalDeviceIndexMapping(PhysicalDeviceType physicalDeviceType);
    virtual ~ShipDeviceIndexToPhysicalDeviceIndexMapping();
    std::string GetMappingId();

    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;

    PhysicalDeviceType GetPhysicalDeviceType();

  protected:
    PhysicalDeviceType mPhysicalDeviceType;
};
} // namespace Ship
