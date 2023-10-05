#pragma once

#include "LUSDeviceIndexToSDLDeviceIndexMapping.h"
#include <unordered_map>
#include <memory>

namespace LUS {
class LUSDeviceIndexMappingManager {
  public:
    LUSDeviceIndexMappingManager();
    ~LUSDeviceIndexMappingManager();

    std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping> CreateDeviceIndexMappingFromConfig(std::string id);
    std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>> GetAllDeviceIndexMappings();
    std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>> GetAllDeviceIndexMappingsFromConfig();
    std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping> GetDeviceIndexMappingFromLUSDeviceIndex(LUSDeviceIndex lusIndex);
    void SetLUSDeviceIndexToPhysicalDeviceIndexMapping(std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping> mapping);
    void RemoveLUSDeviceIndexToPhysicalDeviceIndexMapping(LUSDeviceIndex index);

    void InitializeMappings();
    void SaveMappingIdsToConfig();
  
  private:
    std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>> mLUSDeviceIndexToPhysicalDeviceIndexMappings;
};
} // namespace LUS
