#include "controller/controldevice/controller/mapping/gcadapter/GCAdapterAxisDirectionToAnyMapping.h"
#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"

namespace Ship {
GCAdapterAxisDirectionToAnyMapping::GCAdapterAxisDirectionToAnyMapping(uint8_t gcAxis, int32_t axisDirection)
    : ControllerInputMapping(PhysicalDeviceType::GCAdapter), mGcAxis(gcAxis), mAxisDirection(axisDirection) {
}

GCAdapterAxisDirectionToAnyMapping::~GCAdapterAxisDirectionToAnyMapping() {
}

std::string GCAdapterAxisDirectionToAnyMapping::GetPhysicalDeviceName() {
    return "GC Adapter";
}

std::string GCAdapterAxisDirectionToAnyMapping::GetPhysicalInputName() {
    std::string stickName;
    switch (mGcAxis) {
        case 0:
            stickName = "Main Stick X";
            break;
        case 1:
            stickName = "Main Stick Y";
            break;
        case 2:
            stickName = "C-Stick X";
            break;
        case 3:
            stickName = "C-Stick Y";
            break;
        default:
            stickName = "Unknown Axis";
            break;
    }

    const char* directionIcon = mAxisDirection > 0 ? ICON_FA_PLUS : ICON_FA_MINUS;
    return StringHelper::Sprintf("%s %s", stickName.c_str(), directionIcon);
}
} // namespace Ship