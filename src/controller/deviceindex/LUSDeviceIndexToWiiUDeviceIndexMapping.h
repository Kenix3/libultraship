#ifdef __WIIU__
#pragma once

#include <cstdint>
#include <string>
#include "LUSDeviceIndexToPhysicalDeviceIndexMapping.h"
#include <padscore/wpad.h>

namespace LUS {

class LUSDeviceIndexToWiiUDeviceIndexMapping : public LUSDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    LUSDeviceIndexToWiiUDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex, bool isGamepad, int32_t deviceChannel,
                                           int32_t extensionType);
    ~LUSDeviceIndexToWiiUDeviceIndexMapping();

    void SaveToConfig() override;
    void EraseFromConfig() override;

    bool IsWiiUGamepad();

    int32_t GetDeviceChannel();
    void SetDeviceChannel(int32_t channel);

    int32_t GetExtensionType();
    void SetExtensionType(int32_t extensionType);

    bool HasEquivalentExtensionType(int32_t extensionType);

    std::string GetWiiUControllerName();

  private:
    bool mIsWiiUGamepad;
    int32_t mDeviceChannel;
    int32_t mExtensionType;
};
} // namespace LUS
#endif
