#pragma once

#include <map>
#include <memory>
#include <string>
#include <cstdint>
#include <queue>
#include <vector>
#include "libultraship/libultra/controller.h"
#include "libultraship/color.h"
#include <unordered_map>
#include "ButtonMapping.h"
#include "ControllerStick.h"

namespace LUS {

class Controller {
  public:
    Controller();
    ~Controller();

    void ReloadAllMappings();

    bool IsConnected() const;
    void Connect();
    void Disconnect();
    void AddButtonMapping(std::shared_ptr<ButtonMapping> mapping);
    void ClearButtonMappings(uint16_t bitmask);
    void ClearAllButtonMappings();
    std::shared_ptr<ControllerStick> GetLeftStick();
    std::shared_ptr<ControllerStick> GetRightStick();
    void ReadToPad(OSContPad* pad);

  private:
    std::vector<std::shared_ptr<ButtonMapping>> mButtonMappings;
    std::shared_ptr<ControllerStick> mLeftStick;
    std::shared_ptr<ControllerStick> mRightStick;

    bool mIsConnected;
    std::deque<OSContPad> mPadBuffer;
};
} // namespace LUS
