#ifdef __WIIU__
#include "WiiUButtonToAnyMapping.h"

#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"

namespace LUS {
WiiUButtonToAnyMapping::WiiUButtonToAnyMapping(LUSDeviceIndex lusDeviceIndex, int32_t wiiuControllerButton)
    : ControllerInputMapping(lusDeviceIndex), WiiUMapping(lusDeviceIndex), mControllerButton(wiiuControllerButton) {
}

WiiUButtonToAnyMapping::~WiiUButtonToAnyMapping() {
}

std::string WiiUButtonToAnyMapping::GetPhysicalInputName() {
    return "button";
}

std::string WiiUButtonToAnyMapping::GetPhysicalDeviceName() {
    return GetWiiUDeviceName();
}

bool WiiUButtonToAnyMapping::PhysicalDeviceIsConnected() {
    return ControllerLoaded();
}
} // namespace LUS
#endif
