#include "LUSDeviceIndexMappingManager.h"
#include <SDL2/SDL.h>
#include <Utils/StringHelper.h>
#include "public/bridge/consolevariablebridge.h"
#include <vector>

namespace LUS {
LUSDeviceIndexMappingManager::LUSDeviceIndexMappingManager() {
}

LUSDeviceIndexMappingManager::~LUSDeviceIndexMappingManager() {
}

void LUSDeviceIndexMappingManager::InitializeMappings() {
    // find all currently attached controllers and map their guids to sdl index
    std::unordered_map<int32_t, std::string> attachedSdlControllerGuids;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (!SDL_IsGameController(i)) {
            continue;
        }

        char guidString[33]; // SDL_GUID_LENGTH + 1 for null terminator
        SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i), guidString, sizeof(guidString));
        attachedSdlControllerGuids[i] = guidString;
    }

    // map all controllers where the sdl index and sdl guid match what's saved in the config
    std::vector<int32_t> sdlIndicesToRemove;
    std::vector<LUSDeviceIndex> lusIndicesToRemove;
    auto mappings = GetAllDeviceIndexMappingsFromConfig();
    for (auto [lusIndex, mapping] : mappings) {
        auto sdlMapping = std::dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(mapping);
        if (sdlMapping == nullptr) {
            continue;
        }

        if (attachedSdlControllerGuids[sdlMapping->GetSDLDeviceIndex()] == sdlMapping->GetJoystickGUID()) {
            sdlMapping->EraseFromConfig();
            SetLUSDeviceIndexToPhysicalDeviceIndexMapping(sdlMapping);
            sdlIndicesToRemove.push_back(sdlMapping->GetSDLDeviceIndex());
            lusIndicesToRemove.push_back(sdlMapping->GetLUSDeviceIndex());
        }
    }
    for (auto index : sdlIndicesToRemove) {
        attachedSdlControllerGuids.erase(index);
    }
    for (auto index : lusIndicesToRemove) {
        mappings.erase(index);
    }
    sdlIndicesToRemove.clear();
    lusIndicesToRemove.clear();

    // map all controllers where just the sdl guid matches what's saved in the config
    for (auto [sdlIndex, sdlGuid] : attachedSdlControllerGuids) {
        // do a nested loop here to ensure we map the lowest sdl index and don't double map
        for (auto [lusIndex, mapping] : mappings) {
            auto sdlMapping = std::dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(mapping);
            if (sdlMapping == nullptr) {
                continue;
            }

            if (sdlGuid == sdlMapping->GetJoystickGUID()) {
                sdlMapping->EraseFromConfig();
                sdlMapping->SetSDLDeviceIndex(sdlIndex);
                SetLUSDeviceIndexToPhysicalDeviceIndexMapping(sdlMapping);
                sdlIndicesToRemove.push_back(sdlMapping->GetSDLDeviceIndex());
                lusIndicesToRemove.push_back(sdlMapping->GetLUSDeviceIndex());
                break;
            }
        }
        for (auto index : lusIndicesToRemove) {
            mappings.erase(index);
        }
        lusIndicesToRemove.clear();
    }
    for (auto index : sdlIndicesToRemove) {
        attachedSdlControllerGuids.erase(index);
    }
    sdlIndicesToRemove.clear();

    // todo: check to see if we've satisfied port 1 mappings
    // if we haven't, pause the game and prompt the player
    // not sure if we want to prompt for every LUS index with a mapping on port 1,
    // just the index with the most mappings, or just the lowest index

    // map any remaining controllers to the lowest available LUS index
    for (auto [sdlIndex, sdlGuid] : attachedSdlControllerGuids) {
        for (uint8_t lusIndex = LUSDeviceIndex::Blue; lusIndex < LUSDeviceIndex::Max; lusIndex++) {
            if (GetDeviceIndexMappingFromLUSDeviceIndex(static_cast<LUSDeviceIndex>(lusIndex)) != nullptr) {
                continue;
            }

            SetLUSDeviceIndexToPhysicalDeviceIndexMapping(std::make_shared<LUSDeviceIndexToSDLDeviceIndexMapping>(static_cast<LUSDeviceIndex>(lusIndex), sdlIndex, sdlGuid));
            break;
        }
    }

    for (auto [id, mapping] : mLUSDeviceIndexToPhysicalDeviceIndexMappings) {
        mapping->SaveToConfig();
    }
    SaveMappingIdsToConfig();
}

void LUSDeviceIndexMappingManager::SaveMappingIdsToConfig() {
    // todo: this efficently (when we build out cvar array support?)
    std::string mappingIdListString = "";
    for (auto [lusIndex, mapping] : mLUSDeviceIndexToPhysicalDeviceIndexMappings) {
        mappingIdListString += mapping->GetMappingId();
        mappingIdListString += ",";
    }

    if (mappingIdListString == "") {
        CVarClear("gControllers.DeviceMappingIds");
    } else {
        CVarSetString("gControllers.DeviceMappingIds", mappingIdListString.c_str());
    }

    CVarSave();
}

std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping> LUSDeviceIndexMappingManager::CreateDeviceIndexMappingFromConfig(std::string id) {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(), "");

    if (mappingClass == "LUSDeviceIndexToSDLDeviceIndexMapping") {
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        int32_t sdlDeviceIndex = CVarGetInteger(StringHelper::Sprintf("%s.SDLDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        std::string sdlJoystickGuid = CVarGetString(StringHelper::Sprintf("%s.SDLJoystickGUID", mappingCvarKey.c_str()).c_str(), "");

        if (lusDeviceIndex < 0 || sdlDeviceIndex < 0 || sdlJoystickGuid == "") {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<LUSDeviceIndexToSDLDeviceIndexMapping>(static_cast<LUSDeviceIndex>(lusDeviceIndex), sdlDeviceIndex, sdlJoystickGuid);
    }

    return nullptr;
}

std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>> LUSDeviceIndexMappingManager::GetAllDeviceIndexMappingsFromConfig() {
    std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>> mappings;

    // todo: this efficently (when we build out cvar array support?)
    // i don't expect it to really be a problem with the small number of mappings we have
    // for each controller (especially compared to include/exclude locations in rando), and
    // the audio editor pattern doesn't work for this because that looks for ids that are either
    // hardcoded or provided by an otr file
    std::stringstream mappingIdsStringStream(CVarGetString("gControllers.DeviceMappingIds", ""));
    std::string mappingIdString;
    while (getline(mappingIdsStringStream, mappingIdString, ',')) {
        auto mapping = CreateDeviceIndexMappingFromConfig(mappingIdString);
        mappings[mapping->GetLUSDeviceIndex()] = mapping;
    }

    return mappings;
}

std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping> LUSDeviceIndexMappingManager::GetDeviceIndexMappingFromLUSDeviceIndex(LUSDeviceIndex lusIndex) {
    if (!mLUSDeviceIndexToPhysicalDeviceIndexMappings.contains(lusIndex)) {
        return nullptr;
    }

    return mLUSDeviceIndexToPhysicalDeviceIndexMappings[lusIndex];
}

std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>> LUSDeviceIndexMappingManager::GetAllDeviceIndexMappings() {
    return mLUSDeviceIndexToPhysicalDeviceIndexMappings;
}

void LUSDeviceIndexMappingManager::SetLUSDeviceIndexToPhysicalDeviceIndexMapping(std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping> mapping) {
    mLUSDeviceIndexToPhysicalDeviceIndexMappings[mapping->GetLUSDeviceIndex()] = mapping;
}

void LUSDeviceIndexMappingManager::RemoveLUSDeviceIndexToPhysicalDeviceIndexMapping(LUSDeviceIndex index) {
    mLUSDeviceIndexToPhysicalDeviceIndexMappings.erase(index);
}

// void ControllerButton::ReloadAllMappingsFromConfig() {
//     mButtonMappings.clear();

//     // todo: this efficently (when we build out cvar array support?)
//     // i don't expect it to really be a problem with the small number of mappings we have
//     // for each controller (especially compared to include/exclude locations in rando), and
//     // the audio editor pattern doesn't work for this because that looks for ids that are either
//     // hardcoded or provided by an otr file
//     const std::string buttonMappingIdsCvarKey =
//         StringHelper::Sprintf("gControllers.Port%d.Buttons.%sButtonMappingIds", mPortIndex + 1,
//                               buttonBitmaskToConfigButtonName[mBitmask].c_str());
//     std::stringstream buttonMappingIdsStringStream(CVarGetString(buttonMappingIdsCvarKey.c_str(), ""));
//     std::string buttonMappingIdString;
//     while (getline(buttonMappingIdsStringStream, buttonMappingIdString, ',')) {
//         LoadButtonMappingFromConfig(buttonMappingIdString);
//     }
// }
} // namespace LUS
