#pragma once

#include <cstdint>
#include "LUSDeviceIndex.h"

namespace LUS {

// todo: move some stuff out of here into a base class to handle
// non-sdl physical devices
class LUSDeviceIndexToSDLDeviceIndexMapping {
  public:
    LUSDeviceIndexToSDLDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex, int32_t sdlDeviceIndex);
    ~LUSDeviceIndexToSDLDeviceIndexMapping();

    int32_t GetSDLDeviceIndex();

  protected:
    LUSDeviceIndex mLUSDeviceIndex;
    int32_t mSDLDeviceIndex;
};
} // namespace LUS
