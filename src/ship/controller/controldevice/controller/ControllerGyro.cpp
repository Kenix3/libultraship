#include "ship/controller/controldevice/controller/ControllerGyro.h"

#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/utils/StringHelper.h"
#include "ship/controller/controldevice/controller/mapping/factories/GyroMappingFactory.h"

namespace Ship {
ControllerGyro::ControllerGyro(uint8_t portIndex) : mPortIndex(portIndex) {
}

ControllerGyro::~ControllerGyro() {
}

std::shared_ptr<ControllerGyroMapping> ControllerGyro::GetGyroMapping() {
    return mGyroMapping;
}

void ControllerGyro::SetGyroMapping(std::shared_ptr<ControllerGyroMapping> mapping) {
    mGyroMapping = mapping;
}

bool ControllerGyro::SetGyroMappingFromRawPress() {
    std::shared_ptr<ControllerGyroMapping> mapping = nullptr;

    mapping = GyroMappingFactory::CreateGyroMappingFromSDLInput(mPortIndex);

    if (mapping == nullptr) {
        return false;
    }

    SetGyroMapping(mapping);
    mapping->SaveToConfig();
    SaveGyroMappingIdToConfig();
    const std::string hasConfigCvarKey =
        StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.HasConfig", mPortIndex + 1);
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(hasConfigCvarKey.c_str(), true);
    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
    return true;
}

void ControllerGyro::UpdatePad(float& x, float& y) {
    if (mGyroMapping == nullptr) {
        return;
    }

    mGyroMapping->UpdatePad(x, y);
}

void ControllerGyro::SaveGyroMappingIdToConfig() {
    const std::string gyroMappingIdCvarKey =
        StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.Gyro.GyroMappingId", mPortIndex + 1);

    if (mGyroMapping == nullptr) {
        Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(gyroMappingIdCvarKey.c_str());
    } else {
        Ship::Context::GetInstance()->GetConsoleVariables()->SetString(gyroMappingIdCvarKey.c_str(),
                                                                       mGyroMapping->GetGyroMappingId().c_str());
    }

    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}

void ControllerGyro::ClearGyroMapping() {
    if (mGyroMapping == nullptr) {
        return;
    }

    mGyroMapping->EraseFromConfig();
    mGyroMapping = nullptr;
    SaveGyroMappingIdToConfig();
}

void ControllerGyro::ReloadGyroMappingFromConfig() {
    const std::string gyroMappingIdCvarKey =
        StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.Gyro.GyroMappingId", mPortIndex + 1);

    std::string id = Ship::Context::GetInstance()->GetConsoleVariables()->GetString(gyroMappingIdCvarKey.c_str(), "");
    if (id == "") {
        mGyroMapping = nullptr;
        return;
    }

    mGyroMapping = GyroMappingFactory::CreateGyroMappingFromConfig(mPortIndex, id);
    mGyroMapping->SaveToConfig();
    SaveGyroMappingIdToConfig();
}

bool ControllerGyro::HasMappingForPhysicalDeviceType(PhysicalDeviceType physicalDeviceType) {
    if (mGyroMapping == nullptr) {
        return false;
    }

    return mGyroMapping->GetPhysicalDeviceType() == physicalDeviceType;
}
} // namespace Ship
