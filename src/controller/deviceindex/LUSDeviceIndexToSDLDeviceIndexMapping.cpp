#include "LUSDeviceIndexToSDLDeviceIndexMapping.h"
#include <Utils/StringHelper.h>
#include "public/bridge/consolevariablebridge.h"

namespace LUS {
LUSDeviceIndexToSDLDeviceIndexMapping::LUSDeviceIndexToSDLDeviceIndexMapping(LUSDeviceIndex lusDeviceIndex,
                                                                             int32_t sdlDeviceIndex,
                                                                             std::string sdlJoystickGuid)
    : LUSDeviceIndexToPhysicalDeviceIndexMapping(lusDeviceIndex), mSDLDeviceIndex(sdlDeviceIndex),
      mSDLJoystickGUID(sdlJoystickGuid) {
}

LUSDeviceIndexToSDLDeviceIndexMapping::~LUSDeviceIndexToSDLDeviceIndexMapping() {
}

int32_t LUSDeviceIndexToSDLDeviceIndexMapping::GetSDLDeviceIndex() {
    return mSDLDeviceIndex;
}

void LUSDeviceIndexToSDLDeviceIndexMapping::SetSDLDeviceIndex(int32_t index) {
    mSDLDeviceIndex = index;
}

std::string LUSDeviceIndexToSDLDeviceIndexMapping::GetJoystickGUID() {
    return mSDLJoystickGUID;
}

std::string LUSDeviceIndexToSDLDeviceIndexMapping::GetMappingId() {
    return StringHelper::Sprintf("LUSI%d-SDLI%d", mLUSDeviceIndex, mSDLDeviceIndex);
}

void LUSDeviceIndexToSDLDeviceIndexMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + GetMappingId();
    CVarSetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(),
                  "LUSDeviceIndexToSDLDeviceIndexMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), mLUSDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.SDLDeviceIndex", mappingCvarKey.c_str()).c_str(), mSDLDeviceIndex);
    CVarSetString(StringHelper::Sprintf("%s.SDLJoystickGUID", mappingCvarKey.c_str()).c_str(),
                  mSDLJoystickGUID.c_str());
    CVarSave();
}

void LUSDeviceIndexToSDLDeviceIndexMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + GetMappingId();

    CVarClear(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLJoystickGUID", mappingCvarKey.c_str()).c_str());

    CVarSave();
}
} // namespace LUS
