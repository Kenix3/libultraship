#pragma once

namespace LUS {

#define LUS_DEVICE_INDEX_VALUES \
    X(Keyboard, -1)             \
    X(Blue, 0)                  \
    X(Red, 1)                   \
    X(Orange, 2)                \
    X(Green, 3)                 \
    X(Purple0, 4)               \
    X(Purple1, 5)               \
    X(Purple2, 6)               \
    X(Purple3, 7)               \
    X(Purple4, 8)               \
    X(Purple5, 9)               \
    X(Purple6, 10)              \
    X(Purple7, 11)              \
    X(Purple8, 12)              \
    X(Purple9, 13)              \
    X(Purple10, 14)             \
    X(Purple11, 15)             \
    X(Purple12, 16)             \
    X(Purple13, 17)             \
    X(Purple14, 18)             \
    X(Purple15, 19)             \
    X(Purple16, 20)             \
    X(Purple17, 21)             \
    X(Purple18, 22)             \
    X(Purple19, 23)             \
    X(Purple20, 24)             \
    X(Purple21, 25)             \
    X(Purple22, 26)             \
    X(Purple23, 27)             \
    X(Purple24, 28)             \
    X(Purple25, 29)             \
    X(Purple26, 30)             \
    X(Purple27, 31)             \
    X(Purple28, 32)             \
    X(Purple29, 33)             \
    X(Purple30, 34)             \
    X(Purple31, 35)             \
    X(Purple32, 36)             \
    X(Purple33, 37)             \
    X(Purple34, 38)             \
    X(Purple35, 39)             \
    X(Max, 40)

#define X(name, value) name = value,
enum LUSDeviceIndex { LUS_DEVICE_INDEX_VALUES };
#undef X

} // namespace LUS
