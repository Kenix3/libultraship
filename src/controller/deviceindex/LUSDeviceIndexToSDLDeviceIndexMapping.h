#pragma once

#include <cstdint>
#include "LUSDeviceIndexToPhysicalDeviceIndexMapping.h"

namespace LUS {

class LUSDeviceIndexToSDLDeviceIndexMapping : public LUSDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    LUSDeviceIndexToSDLDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex, int32_t sdlDeviceIndex);
    ~LUSDeviceIndexToSDLDeviceIndexMapping();

    int32_t GetPhysicalDeviceIndex() override;

  private:
    int32_t mSDLDeviceIndex;
};
} // namespace LUS
