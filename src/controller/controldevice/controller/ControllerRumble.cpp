#include "ControllerRumble.h"

#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>
#include <sstream>

#include "controller/controldevice/controller/mapping/factories/RumbleMappingFactory.h"

namespace LUS {
ControllerRumble::ControllerRumble(uint8_t portIndex) : mPortIndex(portIndex) {
}

ControllerRumble::~ControllerRumble() {
}

void ControllerRumble::StartRumble() {
    for (auto [id, mapping] : mRumbleMappings) {
        mapping->StartRumble();
    }
}

void ControllerRumble::StopRumble() {
    for (auto [id, mapping] : mRumbleMappings) {
        mapping->StopRumble();
    }
}

void ControllerRumble::SaveRumbleMappingIdsToConfig() {
    // todo: this efficently (when we build out cvar array support?)
    std::string rumbleMappingIdListString = "";
    for (auto [id, mapping] : mRumbleMappings) {
        rumbleMappingIdListString += id;
        rumbleMappingIdListString += ",";
    }

    const std::string rumbleMappingIdsCvarKey =
        StringHelper::Sprintf("gControllers.Port%d.RumbleMappingIds", mPortIndex + 1);
    if (rumbleMappingIdListString == "") {
        CVarClear(rumbleMappingIdsCvarKey.c_str());
    } else {
        CVarSetString(rumbleMappingIdsCvarKey.c_str(), rumbleMappingIdListString.c_str());
    }

    CVarSave();
}

void ControllerRumble::AddRumbleMapping(std::shared_ptr<ControllerRumbleMapping> mapping) {
    mRumbleMappings[mapping->GetRumbleMappingId()] = mapping;
}

void ControllerRumble::ClearRumbleMappingId(std::string id) {
    mRumbleMappings.erase(id);
    SaveRumbleMappingIdsToConfig();
}

void ControllerRumble::ClearRumbleMapping(std::string id) {
    mRumbleMappings[id]->EraseFromConfig();
    mRumbleMappings.erase(id);
    SaveRumbleMappingIdsToConfig();
}

void ControllerRumble::ClearAllMappings() {
    for (auto [id, mapping] : mRumbleMappings) {
        mapping->EraseFromConfig();
    }
    mRumbleMappings.clear();
    SaveRumbleMappingIdsToConfig();
}

void ControllerRumble::ClearAllMappingsForDevice(LUSDeviceIndex lusDeviceIndex) {
    std::vector<std::string> mappingIdsToRemove;
    for (auto [id, mapping] : mRumbleMappings) {
        if (mapping->GetLUSDeviceIndex() == lusDeviceIndex) {
            mapping->EraseFromConfig();
            mappingIdsToRemove.push_back(id);
        }
    }

    for (auto id : mappingIdsToRemove) {
        auto it = mRumbleMappings.find(id);
        if (it != mRumbleMappings.end()) {
            mRumbleMappings.erase(it);
        }
    }
    SaveRumbleMappingIdsToConfig();
}

#ifdef __WIIU__
void ControllerRumble::AddDefaultMappings(LUSDeviceIndex lusDeviceIndex) {
    for (auto mapping : RumbleMappingFactory::CreateDefaultWiiURumbleMappings(lusDeviceIndex, mPortIndex)) {
        AddRumbleMapping(mapping);
    }

    for (auto [id, mapping] : mRumbleMappings) {
        mapping->SaveToConfig();
    }
    SaveRumbleMappingIdsToConfig();
}
#else
void ControllerRumble::AddDefaultMappings(LUSDeviceIndex lusDeviceIndex) {
    for (auto mapping : RumbleMappingFactory::CreateDefaultSDLRumbleMappings(lusDeviceIndex, mPortIndex)) {
        AddRumbleMapping(mapping);
    }

    for (auto [id, mapping] : mRumbleMappings) {
        mapping->SaveToConfig();
    }
    SaveRumbleMappingIdsToConfig();
}
#endif

void ControllerRumble::LoadRumbleMappingFromConfig(std::string id) {
    auto mapping = RumbleMappingFactory::CreateRumbleMappingFromConfig(mPortIndex, id);

    if (mapping == nullptr) {
        return;
    }

    AddRumbleMapping(mapping);
}

void ControllerRumble::ReloadAllMappingsFromConfig() {
    mRumbleMappings.clear();

    // todo: this efficently (when we build out cvar array support?)
    // i don't expect it to really be a problem with the small number of mappings we have
    // for each controller (especially compared to include/exclude locations in rando), and
    // the audio editor pattern doesn't work for this because that looks for ids that are either
    // hardcoded or provided by an otr file
    const std::string rumbleMappingIdsCvarKey =
        StringHelper::Sprintf("gControllers.Port%d.RumbleMappingIds", mPortIndex + 1);
    std::stringstream rumbleMappingIdsStringStream(CVarGetString(rumbleMappingIdsCvarKey.c_str(), ""));
    std::string rumbleMappingIdString;
    while (getline(rumbleMappingIdsStringStream, rumbleMappingIdString, ',')) {
        LoadRumbleMappingFromConfig(rumbleMappingIdString);
    }
}

std::unordered_map<std::string, std::shared_ptr<ControllerRumbleMapping>> ControllerRumble::GetAllRumbleMappings() {
    return mRumbleMappings;
}

#ifdef __WIIU__
bool ControllerRumble::AddRumbleMappingFromRawPress() {
    std::shared_ptr<ControllerRumbleMapping> mapping = nullptr;

    mapping = RumbleMappingFactory::CreateRumbleMappingFromWiiUInput(mPortIndex);

    if (mapping == nullptr) {
        return false;
    }

    AddRumbleMapping(mapping);
    mapping->SaveToConfig();
    SaveRumbleMappingIdsToConfig();
    const std::string hasConfigCvarKey = StringHelper::Sprintf("gControllers.Port%d.HasConfig", mPortIndex + 1);
    CVarSetInteger(hasConfigCvarKey.c_str(), true);
    CVarSave();
    return true;
}
#else
bool ControllerRumble::AddRumbleMappingFromRawPress() {
    std::shared_ptr<ControllerRumbleMapping> mapping = nullptr;

    mapping = RumbleMappingFactory::CreateRumbleMappingFromSDLInput(mPortIndex);

    if (mapping == nullptr) {
        return false;
    }

    AddRumbleMapping(mapping);
    mapping->SaveToConfig();
    SaveRumbleMappingIdsToConfig();
    const std::string hasConfigCvarKey = StringHelper::Sprintf("gControllers.Port%d.HasConfig", mPortIndex + 1);
    CVarSetInteger(hasConfigCvarKey.c_str(), true);
    CVarSave();
    return true;
}
#endif

bool ControllerRumble::HasMappingsForLUSDeviceIndex(LUSDeviceIndex lusIndex) {
    return std::any_of(mRumbleMappings.begin(), mRumbleMappings.end(),
                       [lusIndex](const auto& mapping) { return mapping.second->GetLUSDeviceIndex() == lusIndex; });
}
} // namespace LUS
