#include "LUSDeviceIndexToSDLDeviceIndexMapping.h"
#include <Utils/StringHelper.h>
#include "public/bridge/consolevariablebridge.h"

namespace LUS {
LUSDeviceIndexToSDLDeviceIndexMapping::LUSDeviceIndexToSDLDeviceIndexMapping(
    LUSDeviceIndex lusDeviceIndex, int32_t sdlDeviceIndex, std::string sdlJoystickGuid, std::string sdlControllerName,
    int32_t stickAxisThresholdPercentage, int32_t triggerAxisThresholdPercentage)
    : LUSDeviceIndexToPhysicalDeviceIndexMapping(lusDeviceIndex), mSDLDeviceIndex(sdlDeviceIndex),
      mSDLJoystickGUID(sdlJoystickGuid), mSDLControllerName(sdlControllerName),
      mStickAxisThresholdPercentage(stickAxisThresholdPercentage),
      mTriggerAxisThresholdPercentage(triggerAxisThresholdPercentage) {
}

LUSDeviceIndexToSDLDeviceIndexMapping::~LUSDeviceIndexToSDLDeviceIndexMapping() {
}

int32_t LUSDeviceIndexToSDLDeviceIndexMapping::GetSDLDeviceIndex() {
    return mSDLDeviceIndex;
}

void LUSDeviceIndexToSDLDeviceIndexMapping::SetSDLDeviceIndex(int32_t index) {
    mSDLDeviceIndex = index;
}

int32_t LUSDeviceIndexToSDLDeviceIndexMapping::GetStickAxisThresholdPercentage() {
    return mStickAxisThresholdPercentage;
}

void LUSDeviceIndexToSDLDeviceIndexMapping::SetStickAxisThresholdPercentage(int32_t stickAxisThresholdPercentage) {
    mStickAxisThresholdPercentage = stickAxisThresholdPercentage;
}

int32_t LUSDeviceIndexToSDLDeviceIndexMapping::GetTriggerAxisThresholdPercentage() {
    return mTriggerAxisThresholdPercentage;
}

void LUSDeviceIndexToSDLDeviceIndexMapping::SetTriggerAxisThresholdPercentage(int32_t triggerAxisThresholdPercentage) {
    mTriggerAxisThresholdPercentage = triggerAxisThresholdPercentage;
}

std::string LUSDeviceIndexToSDLDeviceIndexMapping::GetJoystickGUID() {
    return mSDLJoystickGUID;
}

std::string LUSDeviceIndexToSDLDeviceIndexMapping::GetSDLControllerName() {
    return mSDLControllerName;
}

void LUSDeviceIndexToSDLDeviceIndexMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + GetMappingId();
    CVarSetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(),
                  "LUSDeviceIndexToSDLDeviceIndexMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), mLUSDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.SDLDeviceIndex", mappingCvarKey.c_str()).c_str(), mSDLDeviceIndex);
    CVarSetString(StringHelper::Sprintf("%s.SDLJoystickGUID", mappingCvarKey.c_str()).c_str(),
                  mSDLJoystickGUID.c_str());
    CVarSetString(StringHelper::Sprintf("%s.SDLControllerName", mappingCvarKey.c_str()).c_str(),
                  mSDLControllerName.c_str());
    CVarSetInteger(StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
                   mStickAxisThresholdPercentage);
    CVarSetInteger(StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
                   mTriggerAxisThresholdPercentage);
    CVarSave();
}

void LUSDeviceIndexToSDLDeviceIndexMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + GetMappingId();

    CVarClear(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLJoystickGUID", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerName", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str());

    CVarSave();
}
} // namespace LUS
