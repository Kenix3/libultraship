#ifdef __WIIU__
#include "SDLMapping.h"
#include <spdlog/spdlog.h>
#include "Context.h"
#include "controller/deviceindex/LUSDeviceIndexToWiiUDeviceIndexMapping.h"

#include <Utils/StringHelper.h>

namespace LUS {
WiiUMapping::WiiUMapping(LUSDeviceIndex lusDeviceIndex) : ControllerMapping(lusDeviceIndex), mDeviceConnected(false) {
}

WiiUMapping::~WiiUMapping() {
}

bool WiiUMapping::CloseController() {
    mDeviceConnected = false;
}

bool WiiUMapping::ControllerConnected() {
    return mDeviceConnected;
}


} // namespace LUS
#endif
