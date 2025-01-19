#pragma once

namespace Ship {

#define PHYSICAL_DEVICE_TYPE_VALUES \
    X(Keyboard, 0)              \
    X(Mouse, 1)                 \
    X(SDLGamepad, 2)            \
    X(Max, 3)

#define X(name, value) name = value,
enum PhysicalDeviceType { PHYSICAL_DEVICE_TYPE_VALUES };
#undef X

} // namespace Ship
