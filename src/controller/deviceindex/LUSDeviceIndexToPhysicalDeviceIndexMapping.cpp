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

std::string LUSDeviceIndexToPhysicalDeviceIndexMapping::GetMappingId() {
#define X(name, value) \
    case name:         \
        return #name;
    switch (mLUSDeviceIndex) {
        LUS_DEVICE_INDEX_VALUES
        default:
            return "Max";
    }
#undef X
}
} // namespace LUS
