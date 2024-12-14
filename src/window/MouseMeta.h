#pragma once

#include <string>

namespace Ship {
enum class MouseBtn { LEFT, MIDDLE, RIGHT, BACKWARD, FORWARD, MOUSE_BTN_COUNT, MOUSE_BTN_UNKNOWN };
static std::string mouseBtnNames[7] = {
    "MouseLeft",
    "MouseMiddle",
    "MouseRight",
    "MouseBackward",
    "MouseForward",
    "MOUSE_BTN_COUNT",
    "MOUSE_BTN_UNKNOWN"
};
} // namespace Ship
