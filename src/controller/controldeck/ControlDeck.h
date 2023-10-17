#pragma once

#include "ControlPort.h"
#include <vector>
#include <config/Config.h>
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "controller/deviceindex/LUSDeviceIndexMappingManager.h"

namespace LUS {

class ControlDeck {
  public:
    ControlDeck();
    ~ControlDeck();

    void Init(uint8_t* controllerBits);
    void WriteToPad(OSContPad* pad);
    OSContPad* GetPads();
    uint8_t* GetControllerBits();
    std::shared_ptr<Controller> GetControllerByPort(uint8_t port);
    void BlockGameInput(int32_t blockId);
    void UnblockGameInput(int32_t blockId);
    bool GamepadGameInputBlocked();
    bool KeyboardGameInputBlocked();
    void SetSinglePlayerMappingMode(bool singlePlayer);

    bool ProcessKeyboardEvent(LUS::KbEventType eventType, LUS::KbScancode scancode);

    std::shared_ptr<LUSDeviceIndexMappingManager> GetDeviceIndexMappingManager();

  private:
    std::vector<std::shared_ptr<ControlPort>> mPorts = {};
    uint8_t* mControllerBits = nullptr;
    OSContPad* mPads;
    bool mSinglePlayerMappingMode;

    bool AllGameInputBlocked();
    std::unordered_map<int32_t, bool> mGameInputBlockers;
    std::shared_ptr<LUSDeviceIndexMappingManager> mDeviceIndexMappingManager;
};
} // namespace LUS
