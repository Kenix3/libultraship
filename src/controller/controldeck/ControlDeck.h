#pragma once

#include "ControlPort.h"
#include <vector>
#include <config/Config.h>
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "controller/deviceindex/LUSDeviceIndexToPhysicalDeviceIndexMapping.h"

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

    bool ProcessKeyboardEvent(LUS::KbEventType eventType, LUS::KbScancode scancode);
    
    std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>> GetAllDeviceIndexMappings();
    std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping> GetDeviceIndexMappingFromLUSDeviceIndex(LUSDeviceIndex lusIndex);
    void SetLUSDeviceIndexToPhysicalDeviceIndexMapping(std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping> mapping);
    void RemoveLUSDeviceIndexToPhysicalDeviceIndexMapping(LUSDeviceIndex index);

  private:
    std::vector<std::shared_ptr<ControlPort>> mPorts = {};
    uint8_t* mControllerBits = nullptr;
    OSContPad* mPads;

    bool AllGameInputBlocked();
    std::unordered_map<int32_t, bool> mGameInputBlockers;
    std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>> mLUSDeviceIndexToPhysicalDeviceIndexMappings;
};
} // namespace LUS
