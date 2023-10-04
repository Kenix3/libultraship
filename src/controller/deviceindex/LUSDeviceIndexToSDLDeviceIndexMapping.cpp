#include "LUSDeviceIndexToSDLDeviceIndexMapping.h"

namespace LUS {
LUSDeviceIndexToSDLDeviceIndexMapping::LUSDeviceIndexToSDLDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex, int32_t sdlDeviceIndex) : LUSDeviceIndexToPhysicalDeviceIndexMapping(lusDeviceIndex), mSDLDeviceIndex(sdlDeviceIndex) {
}

LUSDeviceIndexToSDLDeviceIndexMapping::~LUSDeviceIndexToSDLDeviceIndexMapping() {
}

int32_t LUSDeviceIndexToSDLDeviceIndexMapping::GetSDLDeviceIndex() {
    return mSDLDeviceIndex;
}
} // namespace LUS
