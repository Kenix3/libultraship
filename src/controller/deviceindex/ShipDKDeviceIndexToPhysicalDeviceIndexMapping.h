#pragma once

#include <cstdint>
#include <string>
#include "ShipDKDeviceIndex.h"

namespace ShipDK {

class ShipDKDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    ShipDKDeviceIndexToPhysicalDeviceIndexMapping(ShipDKDeviceIndex shipDKDeviceIndex);
    virtual ~ShipDKDeviceIndexToPhysicalDeviceIndexMapping();
    std::string GetMappingId();

    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;

    ShipDKDeviceIndex GetShipDKDeviceIndex();

  protected:
    ShipDKDeviceIndex mShipDKDeviceIndex;
};
} // namespace ShipDK
