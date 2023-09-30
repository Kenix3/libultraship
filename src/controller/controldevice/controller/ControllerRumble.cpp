#include "ControllerRumble.h"

#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>

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

void ControllerRumble::ResetToDefaultMappings(bool sdl, int32_t sdlControllerIndex) {
    ClearAllMappings();
    if (!sdl) {
        return;
    }

    for (auto mapping : RumbleMappingFactory::CreateDefaultSDLRumbleMappings(mPortIndex, sdlControllerIndex)) {
        AddRumbleMapping(mapping);
    }

    for (auto [id, mapping] : mRumbleMappings) {
        mapping->SaveToConfig();
    }
    SaveRumbleMappingIdsToConfig();
}
} // namespace LUS
