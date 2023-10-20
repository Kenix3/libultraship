#pragma once

#include <cstdint>
#include <string>
#include "LUSDeviceIndexToPhysicalDeviceIndexMapping.h"

namespace LUS {

class LUSDeviceIndexToSDLDeviceIndexMapping : public LUSDeviceIndexToPhysicalDeviceIndexMapping {
  public:
    LUSDeviceIndexToSDLDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex, int32_t sdlDeviceIndex,
                                          std::string sdlJoystickGuid, std::string sdlControllerName);
    ~LUSDeviceIndexToSDLDeviceIndexMapping();
    std::string GetMappingId() override;
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
    
};
} // namespace LUS
