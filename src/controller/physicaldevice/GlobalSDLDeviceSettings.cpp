#include "GlobalSDLDeviceSettings.h"

#include <string>
#include "utils/StringHelper.h"
#include "public/bridge/consolevariablebridge.h"

namespace Ship {
GlobalSDLDeviceSettings::GlobalSDLDeviceSettings() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".GlobalSDLDeviceSettings";
    const int32_t defaultAxisThresholdPercentage = 25;
    mStickAxisThresholdPercentage =
        CVarGetInteger(StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
                       defaultAxisThresholdPercentage);
    mTriggerAxisThresholdPercentage =
        CVarGetInteger(StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
                       defaultAxisThresholdPercentage);
}

GlobalSDLDeviceSettings::~GlobalSDLDeviceSettings() {
}

int32_t GlobalSDLDeviceSettings::GetStickAxisThresholdPercentage() {
    return mStickAxisThresholdPercentage;
}

void GlobalSDLDeviceSettings::SetStickAxisThresholdPercentage(int32_t stickAxisThresholdPercentage) {
    mStickAxisThresholdPercentage = stickAxisThresholdPercentage;
}

int32_t GlobalSDLDeviceSettings::GetTriggerAxisThresholdPercentage() {
    return mTriggerAxisThresholdPercentage;
}

void GlobalSDLDeviceSettings::SetTriggerAxisThresholdPercentage(int32_t triggerAxisThresholdPercentage) {
    mTriggerAxisThresholdPercentage = triggerAxisThresholdPercentage;
}

void GlobalSDLDeviceSettings::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".GlobalSDLDeviceSettings";
    CVarSetInteger(StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
                   mStickAxisThresholdPercentage);
    CVarSetInteger(StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
                   mTriggerAxisThresholdPercentage);
    CVarSave();
}

void GlobalSDLDeviceSettings::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".GlobalSDLDeviceSettings";
    CVarClear(StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str());

    CVarSave();
}
} // namespace Ship
