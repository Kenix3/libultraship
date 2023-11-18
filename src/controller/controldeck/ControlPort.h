#pragma once

#include "controller/controldevice/ControlDevice.h"
#include "controller/controldevice/controller/Controller.h"
#include <memory>

namespace LUS {
class ControlPort {
  public:
    ControlPort(uint8_t portIndex);
    ControlPort(uint8_t portIndex, std::shared_ptr<ControlDevice> device);
    ~ControlPort();

    void Connect(std::shared_ptr<ControlDevice> device);
    void Disconnect();

    std::shared_ptr<ControlDevice> GetConnectedDevice();
    std::shared_ptr<Controller> GetConnectedController();

  private:
    uint8_t mPortIndex;
    std::shared_ptr<ControlDevice> mDevice;
};
} // namespace LUS
