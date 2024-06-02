#pragma once

#include "ControlPort.h"
#include <vector>
#include <config/Config.h>
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "controller/deviceindex/ShipDeviceIndexMappingManager.h"

namespace Ship {

class ControlDeck {
  public:
    ControlDeck();
    ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks);
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
    bool IsSinglePlayerMappingMode();

    bool ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode);

    std::shared_ptr<ShipDeviceIndexMappingManager> GetDeviceIndexMappingManager();

  private:
    std::vector<std::shared_ptr<ControlPort>> mPorts = {};
    uint8_t* mControllerBits = nullptr;
    OSContPad* mPads;
    bool mSinglePlayerMappingMode;

    bool AllGameInputBlocked();
    std::unordered_map<int32_t, bool> mGameInputBlockers;
    std::shared_ptr<ShipDeviceIndexMappingManager> mDeviceIndexMappingManager;
};
} // namespace Ship
