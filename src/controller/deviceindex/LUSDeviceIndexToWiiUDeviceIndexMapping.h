#ifdef __WIIU__
#pragma once

#include <cstdint>
#include <string>
#include "LUSDeviceIndexToPhysicalDeviceIndexMapping.h"

namespace LUS {

class LUSDeviceIndexToSDLDeviceIndexMapping : public LUSDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    LUSDeviceIndexToSDLDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex, int32_t sdlDeviceIndex,
                                          std::string sdlJoystickGuid, std::string sdlControllerName, float stickAxisThreshold, float triggerAxisThreshold);
    ~LUSDeviceIndexToSDLDeviceIndexMapping();
    std::string GetJoystickGUID();
    std::string GetSDLControllerName();

    void SaveToConfig() override;
    void EraseFromConfig() override;

    int32_t GetSDLDeviceIndex();
    void SetSDLDeviceIndex(int32_t index);

  private:
    int32_t mSDLDeviceIndex;
    std::string mSDLJoystickGUID;
    std::string mSDLControllerName;
    float mStickAxisThreshold;
    float mTriggerAxisThreshold;
};
} // namespace LUS
#endif
