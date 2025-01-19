#pragma once

#include <cstdint>
#include <string>
#include "ShipDeviceType.h"

namespace Ship {

class ShipDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    ShipDeviceIndexToPhysicalDeviceIndexMapping(ShipDeviceType shipDeviceType);
    virtual ~ShipDeviceIndexToPhysicalDeviceIndexMapping();
    std::string GetMappingId();

    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;

    ShipDeviceType GetShipDeviceType();

  protected:
  ShipDeviceType mShipDeviceType;
};
} // namespace Ship
