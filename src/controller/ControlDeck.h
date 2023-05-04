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
    void SetPhysicalDevice(int32_t virtualIndex, int32_t physicalIndex);
    std::shared_ptr<Controller> GetPhysicalDevice(int32_t physicalIndex);
    std::shared_ptr<Controller> GetPhysicalDeviceFromVirtualIndex(int32_t virtualIndex);
    size_t GetNumPhysicalDevices();
    int32_t GetPhysicalIndexFromVirtualIndex(int32_t slot);
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
