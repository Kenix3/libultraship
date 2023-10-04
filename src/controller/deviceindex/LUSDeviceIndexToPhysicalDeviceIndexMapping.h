#pragma once

#include <cstdint>
#include "LUSDeviceIndex.h"

namespace LUS {

class LUSDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    LUSDeviceIndexToPhysicalDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex);
    ~LUSDeviceIndexToPhysicalDeviceIndexMapping();

    LUSDeviceIndex GetLUSDeviceIndex();

    // todo: figure out if int32 works for non-SDL implementations
    virtual int32_t GetPhysicalDeviceIndex() = 0;

  protected:
    LUSDeviceIndex mLUSDeviceIndex;
};
} // namespace LUS
