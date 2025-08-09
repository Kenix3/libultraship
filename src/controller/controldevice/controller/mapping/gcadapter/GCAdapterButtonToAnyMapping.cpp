#include "GCAdapterButtonToAnyMapping.h"

#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"

namespace Ship {
GCAdapterButtonToAnyMapping::GCAdapterButtonToAnyMapping(CONTROLLERBUTTONS_T gcButton)
    : ControllerInputMapping(PhysicalDeviceType::GCAdapter) {
    mGcButton = gcButton;
}

GCAdapterButtonToAnyMapping::~GCAdapterButtonToAnyMapping() {
}

std::string GCAdapterButtonToAnyMapping::GetPhysicalInputName() {
    switch (mGcButton) {
        case PAD_BUTTON_A:
            return "A";
        case PAD_BUTTON_B:
            return "B";
        case PAD_BUTTON_X:
            return "X";
        case PAD_BUTTON_Y:
            return "Y";
        case PAD_BUTTON_START:
            return StringHelper::Sprintf("%s", ICON_FA_BARS);
        case PAD_BUTTON_UP:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_UP);
        case PAD_BUTTON_DOWN:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_DOWN);
        case PAD_BUTTON_LEFT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_LEFT);
        case PAD_BUTTON_RIGHT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_RIGHT);
        default:
            break;
    }

    return GetGenericButtonName();
}

std::string GCAdapterButtonToAnyMapping::GetGenericButtonName() {
    return StringHelper::Sprintf("B%d", mGcButton);
}

std::string GCAdapterButtonToAnyMapping::GetPhysicalDeviceName() {
    return "GC Adapter";
}
} // namespace Ship
