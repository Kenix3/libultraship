#ifdef __WIIU__
#include "LUSDeviceIndexToWiiUDeviceIndexMapping.h"
#include <Utils/StringHelper.h>
#include "public/bridge/consolevariablebridge.h"

namespace LUS {
LUSDeviceIndexToWiiUDeviceIndexMapping::LUSDeviceIndexToWiiUDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex, int32_t deviceChannel, WPADExtensionType extensionType) 
    : LUSDeviceIndexToPhysicalDeviceIndexMapping(lusDeviceIndex), mDeviceChannel(deviceChannel), mExtensionType(extensionType) {}

LUSDeviceIndexToWiiUDeviceIndexMapping::~LUSDeviceIndexToWiiUDeviceIndexMapping() {
}

int32_t LUSDeviceIndexToWiiUDeviceIndexMapping::GetDeviceChannel() {
    return mDeviceChannel;
}

void LUSDeviceIndexToWiiUDeviceIndexMapping::SetDeviceChannel(int32_t channel) {
    mDeviceChannel = channel;
}

void LUSDeviceIndexToWiiUDeviceIndexMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + GetMappingId();
    CVarSetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(),
                  "LUSDeviceIndexToWiiUDeviceIndexMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), mLUSDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.WiiUDeviceChannel", mappingCvarKey.c_str()).c_str(), mDeviceChannel);
    CVarSetInteger(StringHelper::Sprintf("%s.WiiUDeviceExtensionType", mappingCvarKey.c_str()).c_str(), mExtensionType);
    CVarSave();
}

void LUSDeviceIndexToWiiUDeviceIndexMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + GetMappingId();

    CVarClear(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.WiiUDeviceChannel", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.WiiUDeviceExtensionType", mappingCvarKey.c_str()).c_str());

    CVarSave();
}
} // namespace LUS
#endif
