#pragma once

#include <cstdint>
#include <string>
#include "LUSDeviceIndex.h"

namespace LUS {

class LUSDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    LUSDeviceIndexToPhysicalDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex);
    virtual ~LUSDeviceIndexToPhysicalDeviceIndexMapping();
    std::string GetMappingId();

    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;

    LUSDeviceIndex GetLUSDeviceIndex();

  protected:
    LUSDeviceIndex mLUSDeviceIndex;
};
} // namespace LUS
