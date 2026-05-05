#include "ship/controller/physicaldevice/GlobalSDLDeviceSettings.h"

#include <string>
#include "ship/utils/StringHelper.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"

namespace Ship {
GlobalSDLDeviceSettings::GlobalSDLDeviceSettings() {
    mConsoleVariable = Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>();
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".GlobalSDLDeviceSettings";
    const int32_t defaultAxisThresholdPercentage = 25;
    mStickAxisThresholdPercentage = mConsoleVariable->GetInteger(
        StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
        defaultAxisThresholdPercentage);
    mTriggerAxisThresholdPercentage =
        mConsoleVariable->GetInteger(
            StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
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
    mConsoleVariable->SetInteger(
        StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
        mStickAxisThresholdPercentage);
    mConsoleVariable->SetInteger(
        StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
        mTriggerAxisThresholdPercentage);
    mConsoleVariable->Save();
}

void GlobalSDLDeviceSettings::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".GlobalSDLDeviceSettings";
    mConsoleVariable->ClearVariable(
        StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(
        StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str());

    mConsoleVariable->Save();
}
} // namespace Ship
