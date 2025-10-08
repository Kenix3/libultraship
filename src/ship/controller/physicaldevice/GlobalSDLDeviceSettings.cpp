#include "ship/controller/physicaldevice/GlobalSDLDeviceSettings.h"

#include <string>
#include "ship/utils/StringHelper.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"

namespace Ship {
GlobalSDLDeviceSettings::GlobalSDLDeviceSettings() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".GlobalSDLDeviceSettings";
    const int32_t defaultAxisThresholdPercentage = 25;
    mStickAxisThresholdPercentage = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(
        StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
        defaultAxisThresholdPercentage);
    mTriggerAxisThresholdPercentage = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(
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
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
        mStickAxisThresholdPercentage);
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(),
        mTriggerAxisThresholdPercentage);
    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}

void GlobalSDLDeviceSettings::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".GlobalSDLDeviceSettings";
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str());

    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}
} // namespace Ship
