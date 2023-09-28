#include "ControlPort.h"

namespace LUS {
ControlPort::ControlPort(uint8_t portIndex) : mPortIndex(portIndex), mDevice(nullptr) {
}

ControlPort::ControlPort(uint8_t portIndex, std::shared_ptr<ControlDevice> device)
    : mPortIndex(portIndex), mDevice(device) {
}

ControlPort::~ControlPort() {
}

void ControlPort::Connect(std::shared_ptr<ControlDevice> device) {
    mDevice = device;
}

void ControlPort::Disconnect() {
    mDevice = nullptr;
}

std::shared_ptr<ControlDevice> ControlPort::GetConnectedDevice() {
    return mDevice;
}

std::shared_ptr<Controller> ControlPort::GetConnectedController() {
    return std::dynamic_pointer_cast<Controller>(mDevice);
}

} // namespace LUS
