#include "ControllerGyro.h"

#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>
#include "controller/controldevice/controller/mapping/factories/GyroMappingFactory.h"

namespace LUS {
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

#ifdef __WIIU__
bool ControllerGyro::SetGyroMappingFromRawPress() {
    std::shared_ptr<ControllerGyroMapping> mapping = nullptr;

    mapping = GyroMappingFactory::CreateGyroMappingFromWiiUInput(mPortIndex);

    if (mapping == nullptr) {
        return false;
    }

    SetGyroMapping(mapping);
    mapping->SaveToConfig();
    SaveGyroMappingIdToConfig();
    const std::string hasConfigCvarKey = StringHelper::Sprintf("gControllers.Port%d.HasConfig", mPortIndex + 1);
    CVarSetInteger(hasConfigCvarKey.c_str(), true);
    CVarSave();
    return true;
}
#else
bool ControllerGyro::SetGyroMappingFromRawPress() {
    std::shared_ptr<ControllerGyroMapping> mapping = nullptr;

    mapping = GyroMappingFactory::CreateGyroMappingFromSDLInput(mPortIndex);

    if (mapping == nullptr) {
        return false;
    }

    SetGyroMapping(mapping);
    mapping->SaveToConfig();
    SaveGyroMappingIdToConfig();
    const std::string hasConfigCvarKey = StringHelper::Sprintf("gControllers.Port%d.HasConfig", mPortIndex + 1);
    CVarSetInteger(hasConfigCvarKey.c_str(), true);
    CVarSave();
    return true;
}
#endif

void ControllerGyro::UpdatePad(float& x, float& y) {
    if (mGyroMapping == nullptr) {
        return;
    }

    mGyroMapping->UpdatePad(x, y);
}

void ControllerGyro::SaveGyroMappingIdToConfig() {
    const std::string gyroMappingIdCvarKey =
        StringHelper::Sprintf("gControllers.Port%d.Gyro.GyroMappingId", mPortIndex + 1);

    if (mGyroMapping == nullptr) {
        CVarClear(gyroMappingIdCvarKey.c_str());
    } else {
        CVarSetString(gyroMappingIdCvarKey.c_str(), mGyroMapping->GetGyroMappingId().c_str());
    }

    CVarSave();
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
        StringHelper::Sprintf("gControllers.Port%d.Gyro.GyroMappingId", mPortIndex + 1);

    std::string id = CVarGetString(gyroMappingIdCvarKey.c_str(), "");
    if (id == "") {
        mGyroMapping = nullptr;
        return;
    }

    mGyroMapping = GyroMappingFactory::CreateGyroMappingFromConfig(mPortIndex, id);
    mGyroMapping->SaveToConfig();
    SaveGyroMappingIdToConfig();
}

bool ControllerGyro::HasMappingForLUSDeviceIndex(LUSDeviceIndex lusIndex) {
    if (mGyroMapping == nullptr) {
        return false;
    }

    return mGyroMapping->GetLUSDeviceIndex() == lusIndex;
}
} // namespace LUS
