#pragma once

#include <cstdint>
#include <string>
#include "ShipDKDeviceIndexToPhysicalDeviceIndexMapping.h"

namespace ShipDK {

class ShipDKDeviceIndexToSDLDeviceIndexMapping : public ShipDKDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    ShipDKDeviceIndexToSDLDeviceIndexMapping(ShipDKDeviceIndex shipDKDeviceIndex, int32_t sdlDeviceIndex,
                                          std::string sdlJoystickGuid, std::string sdlControllerName,
                                          int32_t stickAxisThresholdPercentage, int32_t triggerAxisThresholdPercentage);
    ~ShipDKDeviceIndexToSDLDeviceIndexMapping();
    std::string GetJoystickGUID();
    std::string GetSDLControllerName();

    void SaveToConfig() override;
    void EraseFromConfig() override;

    int32_t GetSDLDeviceIndex();
    void SetSDLDeviceIndex(int32_t index);

    int32_t GetStickAxisThresholdPercentage();
    void SetStickAxisThresholdPercentage(int32_t stickAxisThresholdPercentage);

    int32_t GetTriggerAxisThresholdPercentage();
    void SetTriggerAxisThresholdPercentage(int32_t triggerAxisThresholdPercentage);

  private:
    int32_t mSDLDeviceIndex;
    std::string mSDLJoystickGUID;
    std::string mSDLControllerName;
    int32_t mStickAxisThresholdPercentage;
    int32_t mTriggerAxisThresholdPercentage;
};
} // namespace ShipDK
