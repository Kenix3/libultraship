#ifdef __WIIU__
#pragma once

#include <cstdint>
#include <string>
#include "LUSDeviceIndexToPhysicalDeviceIndexMapping.h"
#include <padscore/wpad.h>

namespace LUS {

class LUSDeviceIndexToWiiUDeviceIndexMapping : public LUSDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    LUSDeviceIndexToWiiUDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex, int32_t deviceChannel, WPADExtensionType extensionType);
    ~LUSDeviceIndexToWiiUDeviceIndexMapping();

    void SaveToConfig() override;
    void EraseFromConfig() override;

    int32_t GetDeviceChannel();
    void SetDeviceChannel(int32_t channel);

  private:
    WPADChan mDeviceChannel;
    WPADExtensionType mExtensionType;
};
} // namespace LUS
#endif
