#include "ship/controller/controldevice/controller/ControllerLED.h"

#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/utils/StringHelper.h"
#include <sstream>
#include <algorithm>

#include "ship/controller/controldevice/controller/mapping/factories/LEDMappingFactory.h"

namespace Ship {
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
    std::string ledMappingIdListString = "";
    for (auto [id, mapping] : mLEDMappings) {
        ledMappingIdListString += id;
        ledMappingIdListString += ",";
    }

    const std::string ledMappingIdsCvarKey =
        StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.LEDMappingIds", mPortIndex + 1);
    if (ledMappingIdsCvarKey == "") {
        Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(ledMappingIdsCvarKey.c_str());
    } else {
        Ship::Context::GetInstance()->GetConsoleVariables()->SetString(ledMappingIdsCvarKey.c_str(),
                                                                       ledMappingIdListString.c_str());
    }

    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}

void ControllerLED::AddLEDMapping(std::shared_ptr<ControllerLEDMapping> mapping) {
    mLEDMappings[mapping->GetLEDMappingId()] = mapping;
}

void ControllerLED::ClearLEDMappingId(std::string id) {
    mLEDMappings.erase(id);
    SaveLEDMappingIdsToConfig();
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

void ControllerLED::ClearAllMappingsForDeviceType(PhysicalDeviceType physicalDeviceType) {
    std::vector<std::string> mappingIdsToRemove;
    for (auto [id, mapping] : mLEDMappings) {
        if (mapping->GetPhysicalDeviceType() == physicalDeviceType) {
            mapping->EraseFromConfig();
            mappingIdsToRemove.push_back(id);
        }
    }

    for (auto id : mappingIdsToRemove) {
        auto it = mLEDMappings.find(id);
        if (it != mLEDMappings.end()) {
            mLEDMappings.erase(it);
        }
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
    const std::string ledMappingIdsCvarKey =
        StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.LEDMappingIds", mPortIndex + 1);
    std::stringstream ledMappingIdsStringStream(
        Ship::Context::GetInstance()->GetConsoleVariables()->GetString(ledMappingIdsCvarKey.c_str(), ""));
    std::string ledMappingIdString;
    while (getline(ledMappingIdsStringStream, ledMappingIdString, ',')) {
        LoadLEDMappingFromConfig(ledMappingIdString);
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
    const std::string hasConfigCvarKey =
        StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.HasConfig", mPortIndex + 1);
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(hasConfigCvarKey.c_str(), true);
    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
    return true;
}

bool ControllerLED::HasMappingsForPhysicalDeviceType(PhysicalDeviceType physicalDeviceType) {
    return std::any_of(mLEDMappings.begin(), mLEDMappings.end(), [physicalDeviceType](const auto& mapping) {
        return mapping.second->GetPhysicalDeviceType() == physicalDeviceType;
    });
}
} // namespace Ship
