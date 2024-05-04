#include "ShipDeviceIndexToSDLDeviceIndexMapping.h"
#include "utils/StringHelper.h"
#include "public/bridge/consolevariablebridge.h"

namespace Ship {
ShipDeviceIndexToSDLDeviceIndexMapping::ShipDeviceIndexToSDLDeviceIndexMapping(
    ShipDeviceIndex shipDeviceIndex, int32_t sdlDeviceIndex, std::string sdlJoystickGuid, std::string sdlControllerName,
    int32_t stickAxisThresholdPercentage, int32_t triggerAxisThresholdPercentage)
    : ShipDeviceIndexToPhysicalDeviceIndexMapping(shipDeviceIndex), mSDLDeviceIndex(sdlDeviceIndex),
      mSDLJoystickGUID(sdlJoystickGuid), mSDLControllerName(sdlControllerName),
      mStickAxisThresholdPercentage(stickAxisThresholdPercentage),
      mTriggerAxisThresholdPercentage(triggerAxisThresholdPercentage) {
}

ShipDeviceIndexToSDLDeviceIndexMapping::~ShipDeviceIndexToSDLDeviceIndexMapping() {
}

int32_t ShipDeviceIndexToSDLDeviceIndexMapping::GetSDLDeviceIndex() {
    return mSDLDeviceIndex;
}

void ShipDeviceIndexToSDLDeviceIndexMapping::SetSDLDeviceIndex(int32_t index) {
    mSDLDeviceIndex = index;
}

int32_t ShipDeviceIndexToSDLDeviceIndexMapping::GetStickAxisThresholdPercentage() {
    return mStickAxisThresholdPercentage;
}

void ShipDeviceIndexToSDLDeviceIndexMapping::SetStickAxisThresholdPercentage(int32_t stickAxisThresholdPercentage) {
    mStickAxisThresholdPercentage = stickAxisThresholdPercentage;
}

int32_t ShipDeviceIndexToSDLDeviceIndexMapping::GetTriggerAxisThresholdPercentage() {
    return mTriggerAxisThresholdPercentage;
}

void ShipDeviceIndexToSDLDeviceIndexMapping::SetTriggerAxisThresholdPercentage(int32_t triggerAxisThresholdPercentage) {
    mTriggerAxisThresholdPercentage = triggerAxisThresholdPercentage;
}

std::string ShipDeviceIndexToSDLDeviceIndexMapping::GetJoystickGUID() {
    return mSDLJoystickGUID;
}

std::string ShipDeviceIndexToSDLDeviceIndexMapping::GetSDLControllerName() {
    return mSDLControllerName;
}

void ShipDeviceIndexToSDLDeviceIndexMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".DeviceMappings." + GetMappingId();
    CVarSetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(),
                  "ShipDeviceIndexToSDLDeviceIndexMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str(), mShipDeviceIndex);
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

void ShipDeviceIndexToSDLDeviceIndexMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".DeviceMappings." + GetMappingId();

    CVarClear(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLJoystickGUID", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerName", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str());

    CVarSave();
}
} // namespace Ship
