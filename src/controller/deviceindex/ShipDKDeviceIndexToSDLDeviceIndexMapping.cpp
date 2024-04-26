#include "ShipDKDeviceIndexToSDLDeviceIndexMapping.h"
#include <Utils/StringHelper.h>
#include "public/bridge/consolevariablebridge.h"

namespace ShipDK {
ShipDKDeviceIndexToSDLDeviceIndexMapping::ShipDKDeviceIndexToSDLDeviceIndexMapping(
    ShipDKDeviceIndex shipDKDeviceIndex, int32_t sdlDeviceIndex, std::string sdlJoystickGuid,
    std::string sdlControllerName, int32_t stickAxisThresholdPercentage, int32_t triggerAxisThresholdPercentage)
    : ShipDKDeviceIndexToPhysicalDeviceIndexMapping(shipDKDeviceIndex), mSDLDeviceIndex(sdlDeviceIndex),
      mSDLJoystickGUID(sdlJoystickGuid), mSDLControllerName(sdlControllerName),
      mStickAxisThresholdPercentage(stickAxisThresholdPercentage),
      mTriggerAxisThresholdPercentage(triggerAxisThresholdPercentage) {
}

ShipDKDeviceIndexToSDLDeviceIndexMapping::~ShipDKDeviceIndexToSDLDeviceIndexMapping() {
}

int32_t ShipDKDeviceIndexToSDLDeviceIndexMapping::GetSDLDeviceIndex() {
    return mSDLDeviceIndex;
}

void ShipDKDeviceIndexToSDLDeviceIndexMapping::SetSDLDeviceIndex(int32_t index) {
    mSDLDeviceIndex = index;
}

int32_t ShipDKDeviceIndexToSDLDeviceIndexMapping::GetStickAxisThresholdPercentage() {
    return mStickAxisThresholdPercentage;
}

void ShipDKDeviceIndexToSDLDeviceIndexMapping::SetStickAxisThresholdPercentage(int32_t stickAxisThresholdPercentage) {
    mStickAxisThresholdPercentage = stickAxisThresholdPercentage;
}

int32_t ShipDKDeviceIndexToSDLDeviceIndexMapping::GetTriggerAxisThresholdPercentage() {
    return mTriggerAxisThresholdPercentage;
}

void ShipDKDeviceIndexToSDLDeviceIndexMapping::SetTriggerAxisThresholdPercentage(
    int32_t triggerAxisThresholdPercentage) {
    mTriggerAxisThresholdPercentage = triggerAxisThresholdPercentage;
}

std::string ShipDKDeviceIndexToSDLDeviceIndexMapping::GetJoystickGUID() {
    return mSDLJoystickGUID;
}

std::string ShipDKDeviceIndexToSDLDeviceIndexMapping::GetSDLControllerName() {
    return mSDLControllerName;
}

void ShipDKDeviceIndexToSDLDeviceIndexMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + GetMappingId();
    CVarSetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(),
                  "ShipDKDeviceIndexToSDLDeviceIndexMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.ShipDKDeviceIndex", mappingCvarKey.c_str()).c_str(), mShipDKDeviceIndex);
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

void ShipDKDeviceIndexToSDLDeviceIndexMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + GetMappingId();

    CVarClear(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.ShipDKDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLJoystickGUID", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerName", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str());

    CVarSave();
}
} // namespace ShipDK
