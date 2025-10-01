#pragma once

#include "ship/controller/controldevice/ControlDevice.h"
#include "ship/controller/controldevice/controller/Controller.h"
#include <memory>

namespace Ship {
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
} // namespace Ship
