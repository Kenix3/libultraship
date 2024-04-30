#pragma once

#include <cstdint>
#include <string>
#include "ShipDeviceIndex.h"

namespace Ship {

class ShipDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    ShipDeviceIndexToPhysicalDeviceIndexMapping(ShipDeviceIndex shipDeviceIndex);
    virtual ~ShipDeviceIndexToPhysicalDeviceIndexMapping();
    std::string GetMappingId();

    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;

    ShipDeviceIndex GetShipDeviceIndex();

  protected:
    ShipDeviceIndex mShipDeviceIndex;
};
} // namespace Ship
