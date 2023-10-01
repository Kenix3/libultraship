#include "ControllerLED.h"

#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>

#include "controller/controldevice/controller/mapping/factories/LEDMappingFactory.h"

namespace LUS {
ControllerLED::ControllerLED(uint8_t portIndex) : mPortIndex(portIndex) {
}

ControllerLED::~ControllerLED() {
}

void ControllerLED::SetLEDColor(Color_RGB8 color) {
    for (auto [id, mapping] : mLEDMappings) {
        mapping->SetLEDColor(color);
    }
}

void ControllerLED::SaveLEDMappingIdsToConfig() {
    // todo: this efficently (when we build out cvar array support?)
    std::string LEDMappingIdListString = "";
    for (auto [id, mapping] : mLEDMappings) {
        LEDMappingIdListString += id;
        LEDMappingIdListString += ",";
    }

    const std::string LEDMappingIdsCvarKey =
        StringHelper::Sprintf("gControllers.Port%d.LEDMappingIds", mPortIndex + 1);
    if (LEDMappingIdsCvarKey == "") {
        CVarClear(LEDMappingIdsCvarKey.c_str());
    } else {
        CVarSetString(LEDMappingIdsCvarKey.c_str(), LEDMappingIdListString.c_str());
    }

    CVarSave();
}

void ControllerLED::AddLEDMapping(std::shared_ptr<ControllerLEDMapping> mapping) {
    mLEDMappings[mapping->GetLEDMappingId()] = mapping;
}

void ControllerLED::ClearLEDMapping(std::string id) {
    mLEDMappings[id]->EraseFromConfig();
    mLEDMappings.erase(id);
    SaveLEDMappingIdsToConfig();
}

void ControllerLED::ClearAllMappings() {
    for (auto [id, mapping] : mLEDMappings) {
        mapping->EraseFromConfig();
    }
    mLEDMappings.clear();
    SaveLEDMappingIdsToConfig();
}

void ControllerLED::ResetToDefaultMappings(bool sdl, int32_t sdlControllerIndex) {
    ClearAllMappings();
    if (!sdl) {
        return;
    }

    for (auto mapping : LEDMappingFactory::CreateDefaultSDLLEDMappings(mPortIndex, sdlControllerIndex)) {
        AddLEDMapping(mapping);
    }

    for (auto [id, mapping] : mLEDMappings) {
        mapping->SaveToConfig();
    }
    SaveLEDMappingIdsToConfig();
}

void ControllerLED::LoadLEDMappingFromConfig(std::string id) {
    auto mapping = LEDMappingFactory::CreateLEDMappingFromConfig(mPortIndex, id);

    if (mapping == nullptr) {
        return;
    }

    AddLEDMapping(mapping);
}

void ControllerLED::ReloadAllMappingsFromConfig() {
    mLEDMappings.clear();

    // todo: this efficently (when we build out cvar array support?)
    // i don't expect it to really be a problem with the small number of mappings we have
    // for each controller (especially compared to include/exclude locations in rando), and
    // the audio editor pattern doesn't work for this because that looks for ids that are either
    // hardcoded or provided by an otr file
    const std::string LEDMappingIdsCvarKey =
        StringHelper::Sprintf("gControllers.Port%d.LEDMappingIds", mPortIndex + 1);
    std::stringstream LEDMappingIdsStringStream(CVarGetString(LEDMappingIdsCvarKey.c_str(), ""));
    std::string LEDMappingIdString;
    while (getline(LEDMappingIdsStringStream, LEDMappingIdString, ',')) {
        LoadLEDMappingFromConfig(LEDMappingIdString);
    }
}

std::unordered_map<std::string, std::shared_ptr<ControllerLEDMapping>> ControllerLED::GetAllLEDMappings() {
    return mLEDMappings;
}

bool ControllerLED::AddLEDMappingFromRawPress() {
    std::shared_ptr<ControllerLEDMapping> mapping = nullptr;

    mapping = LEDMappingFactory::CreateLEDMappingFromSDLInput(mPortIndex);

    if (mapping == nullptr) {
        return false;
    }

    AddLEDMapping(mapping);
    mapping->SaveToConfig();
    SaveLEDMappingIdsToConfig();
    return true;
}
} // namespace LUS
