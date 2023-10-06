#include "ControllerMapping.h"

namespace LUS {
ControllerMapping::ControllerMapping(LUSDeviceIndex lusDeviceIndex) : mLUSDeviceIndex(lusDeviceIndex) {
}

ControllerMapping::~ControllerMapping() {
}

std::string ControllerMapping::GetPhysicalDeviceName() {
    return "Unknown";
}

LUSDeviceIndex ControllerMapping::GetLUSDeviceIndex() {
    return mLUSDeviceIndex;
}
} // namespace LUS
