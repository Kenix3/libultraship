#pragma once

#include "DeviceAttachment.h"
#include <memory>

namespace LUS {
class AttachmentPort {
  public:
    AttachmentPort();
    AttachmentPort(std::shared_ptr<DeviceAttachment> attachment);
    ~AttachmentPort();

    void Connect(std::shared_ptr<DeviceAttachment> attachment);
    void Disconnect();

  private:
    std::shared_ptr<DeviceAttachment> mAttachment;
};
}
