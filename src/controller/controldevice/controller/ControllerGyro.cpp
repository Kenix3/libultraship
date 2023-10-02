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

// void ControllerGyro::AddOrReplaceGyroMapping(std::shared_ptr<ControllerGyroMapping> mapping) {
//     mButtonMappings[mapping->GetButtonMappingId()] = mapping;
// }

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
} // namespace LUS
