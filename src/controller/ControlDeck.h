#pragma once

#include "Controller.h"
#include <vector>
#include <Mercury.h>

namespace Ship {

class ControlDeck {
  public:
    void Init(uint8_t* controllerBits);
    void ScanPhysicalDevices();
    void WriteToPad(OSContPad* pad) const;
    void LoadControllerSettings();
    void SaveControllerSettings();
    void SetPhysicalDevice(int32_t slot, int32_t deviceSlot);
    std::shared_ptr<Controller> GetPhysicalDevice(int32_t deviceSlot);
    std::shared_ptr<Controller> GetPhysicalDeviceFromVirtualSlot(int32_t slot);
    size_t GetNumPhysicalDevices();
    int32_t GetVirtualDevice(int32_t slot);
    size_t GetNumVirtualDevices();
    uint8_t* GetControllerBits();
    void BlockGameInput();
    void UnblockGameInput();
    bool ShouldBlockGameInput(std::string inputDeviceGuid) const;

  private:
    std::vector<int32_t> mVirtualDevices = {};
    std::vector<std::shared_ptr<Controller>> mPhysicalDevices = {};
    uint8_t* mControllerBits = nullptr;
    bool mShouldBlockGameInput = false;
};
} // namespace Ship
