#pragma once

#include <cstdint>
#include <string>
#include "ShipDeviceIndexToPhysicalDeviceIndexMapping.h"

namespace Ship {

class ShipDeviceIndexToSDLDeviceIndexMapping : public ShipDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    ShipDeviceIndexToSDLDeviceIndexMapping(ShipDeviceIndex shipDeviceIndex, int32_t sdlDeviceIndex,
                                           std::string sdlJoystickGuid, std::string sdlControllerName,
                                           int32_t stickAxisThresholdPercentage,
                                           int32_t triggerAxisThresholdPercentage);
    ~ShipDeviceIndexToSDLDeviceIndexMapping();
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
} // namespace Ship
