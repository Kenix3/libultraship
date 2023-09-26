#include "AttachmentPort.h"

namespace LUS {
AttachmentPort::AttachmentPort() : mAttachment(nullptr) {
}

AttachmentPort::AttachmentPort(std::shared_ptr<DeviceAttachment> attachment) : mAttachment(attachment) {
}

AttachmentPort::~AttachmentPort() {
}

void AttachmentPort::Connect(std::shared_ptr<DeviceAttachment> attachment) {
    mAttachment = attachment;
}

void AttachmentPort::Disconnect() {
    mAttachment = nullptr;
}

} // namespace LUS
