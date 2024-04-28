#ifdef __WIIU__
#pragma once

#include <cstdint>
#include <string>
#include "ShipDeviceIndexToPhysicalDeviceIndexMapping.h"
#include <padscore/wpad.h>

namespace Ship {

class ShipDeviceIndexToWiiUDeviceIndexMapping : public ShipDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    ShipDeviceIndexToWiiUDeviceIndexMapping(ShipDeviceIndex shipDeviceIndex, bool isGamepad, int32_t deviceChannel,
                                            int32_t extensionType);
    ~ShipDeviceIndexToWiiUDeviceIndexMapping();

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
} // namespace Ship
#endif
