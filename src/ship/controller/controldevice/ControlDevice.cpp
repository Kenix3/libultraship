#include "ControlDevice.h"

namespace Ship {
ControlDevice::ControlDevice(uint8_t portIndex) : mPortIndex(portIndex) {
}

ControlDevice::~ControlDevice() {
}

} // namespace Ship
