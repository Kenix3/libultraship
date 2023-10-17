#include "LUSDeviceIndexToPhysicalDeviceIndexMapping.h"

namespace LUS {
LUSDeviceIndexToPhysicalDeviceIndexMapping::LUSDeviceIndexToPhysicalDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex)
    : mLUSDeviceIndex(lusDeviceIndex) {
}

LUSDeviceIndexToPhysicalDeviceIndexMapping::~LUSDeviceIndexToPhysicalDeviceIndexMapping() {
}

LUSDeviceIndex LUSDeviceIndexToPhysicalDeviceIndexMapping::GetLUSDeviceIndex() {
    return mLUSDeviceIndex;
}
} // namespace LUS
