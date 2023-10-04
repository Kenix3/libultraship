#pragma once

#include <cstdint>
#include "LUSDeviceIndex.h"

namespace LUS {

class LUSDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    LUSDeviceIndexToPhysicalDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex);
    virtual ~LUSDeviceIndexToPhysicalDeviceIndexMapping();

    LUSDeviceIndex GetLUSDeviceIndex();

  protected:
    LUSDeviceIndex mLUSDeviceIndex;
};
} // namespace LUS
