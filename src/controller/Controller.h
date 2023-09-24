#pragma once

#include <map>
#include <memory>
#include <string>
#include <cstdint>
#include <queue>
#include <vector>
#include <map>
#include "libultraship/libultra/controller.h"
#include "libultraship/color.h"
#include <unordered_map>
#include "ButtonMapping.h"
#include "ControllerStick.h"
#include "ControllerGyro.h"

namespace LUS {

class Controller {
  public:
    Controller(uint8_t port);
    ~Controller();

    void ReloadAllMappings();

    bool IsConnected() const;
    void Connect();
    void Disconnect();
    void AddButtonMapping(std::shared_ptr<ButtonMapping> mapping);
    void ClearButtonMapping(std::string uuid);
    void ClearButtonMapping(std::shared_ptr<ButtonMapping> mapping);
    void ClearAllButtonMappings();
    std::unordered_map<std::string, std::shared_ptr<ButtonMapping>> GetAllButtonMappings();
    std::shared_ptr<ControllerStick> GetLeftStick();
    std::shared_ptr<ControllerStick> GetRightStick();
    std::shared_ptr<ControllerGyro> GetGyro();
    void ReadToPad(OSContPad* pad);

  private:
    std::unordered_map<std::string, std::shared_ptr<ButtonMapping>> mButtonMappings;
    std::shared_ptr<ControllerStick> mLeftStick;
    std::shared_ptr<ControllerStick> mRightStick;
    std::shared_ptr<ControllerGyro> mGyro;

    bool mIsConnected;
    uint8_t mPort;

    std::deque<OSContPad> mPadBuffer;
};
} // namespace LUS
