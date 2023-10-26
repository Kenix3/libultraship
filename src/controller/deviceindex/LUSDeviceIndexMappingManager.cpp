#include "LUSDeviceIndexMappingManager.h"
#include <SDL2/SDL.h>
#include <Utils/StringHelper.h>
#include "public/bridge/consolevariablebridge.h"
#include <vector>
#include "Context.h"
#include "ControllerDisconnectedWindow.h"
#include "ControllerReorderingWindow.h"

#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/wiiu/WiiUMapping.h"
#include "WiiUImpl.h"
#else
#include "controller/controldevice/controller/mapping/sdl/SDLMapping.h"
#endif

#include <sstream>

namespace LUS {
LUSDeviceIndexMappingManager::LUSDeviceIndexMappingManager() : mIsInitialized(false) {
    UpdateControllerNamesFromConfig();
}

LUSDeviceIndexMappingManager::~LUSDeviceIndexMappingManager() {
}

#ifdef __WIIU__
void LUSDeviceIndexMappingManager::InitializeMappingsMultiplayer(std::vector<int32_t> wiiuDeviceChannels) {
    for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
        for (auto mapping :
             Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex)->GetAllMappings()) {
            auto wiiuMapping = std::dynamic_pointer_cast<WiiUMapping>(mapping);
            if (wiiuMapping == nullptr) {
                continue;
            }

            wiiuMapping->CloseController();
        }
    }
    mLUSDeviceIndexToPhysicalDeviceIndexMappings.clear();
    uint8_t port = 0;
    for (auto channel : wiiuDeviceChannels) {
        InitializeSDLMappingsForPort(port, sdlIndex);
        port++;
    }
    mIsInitialized = true;
}

void LUSDeviceIndexMappingManager::InitializeWiiUMappingsForPort(uint8_t n64port, bool isGamepad, int32_t wiiuChannel) {
    KPADError error;
    KPADStatus* status = LUS::WiiU::GetKPADStatus(wiiuChannel, &kpadError);

    if (!isGamepad && status == nullptr) {
        return;
    }

    std::vector<LUSDeviceIndex> matchingLusIndices;
    auto mappings = GetAllDeviceIndexMappingsFromConfig();
    for (auto [lusIndex, mapping] : mappings) {
        auto wiiuMapping = std::dynamic_pointer_cast<LUSDeviceIndexToWiiUDeviceIndexMapping>(mapping);
        if (wiiuMapping == nullptr) {
            continue;
        }

        if (isGamepad && wiiuMapping->IsWiiUGamepad()) {
            matchingLusIndices.push_back(lusIndex);
            continue;
        }

        if (wiiuMapping->HasEquivalentExtensionType(status->extensionType)) {
            matchingLusIndices.push_back(lusIndex);
        }
    }

    // set this device to the lowest available lus index with a compatable extension type
    for (auto lusIndex : matchingLusIndices) {
        if (GetDeviceIndexMappingFromLUSDeviceIndex(lusIndex) != nullptr) {
            // we already loaded this one
            continue;
        }

        auto mapping = mappings[lusIndex];
        auto wiiuMapping = std::dynamic_pointer_cast<LUSDeviceIndexToWiiUDeviceIndexMapping>(mapping);

        if (!isGamepad) {
            wiiuMapping->SetDeviceChannel(wiiuChannel);
        }
        SetLUSDeviceIndexToPhysicalDeviceIndexMapping(wiiuMapping);

        // if we have mappings for this LUS device on this port, we're good and don't need to move any mappings
        if (Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->HasMappingsForLUSDeviceIndex(
                lusIndex)) {
            return;
        }

        // move mappings from other port to this one
        for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
            if (portIndex == n64port) {
                continue;
            }

            if (!Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex)->HasMappingsForLUSDeviceIndex(
                    lusIndex)) {
                continue;
            }

            Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex)->MoveMappingsToDifferentController(
                Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port), lusIndex);
            return;
        }

        // we shouldn't get here
        return;
    }

    // if we didn't find a mapping, make defaults
    auto lusIndex = GetLowestLUSDeviceIndexWithNoAssociatedButtonOrAxisDirectionMappings();
    auto deviceIndexMapping =
        std::make_shared<LUSDeviceIndexToWiiUDeviceIndexMapping>(lusIndex, wiiuChannel, isGamepad, !isGamepad ? status->extensionType : -1);
    mLUSDeviceIndexToWiiUDeviceTypes[lusIndex] = {isGamepad, !isGamepad ? status->extensionType : -1};
    deviceIndexMapping->SaveToConfig();
    SetLUSDeviceIndexToPhysicalDeviceIndexMapping(deviceIndexMapping);
    SaveMappingIdsToConfig();
    Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->AddDefaultMappings(lusIndex);
}

bool LUSDeviceIndexMappingManager::IsValidWiiUExtensionType(int32_t extensionType) {
    switch (extensionType) {
        case WPAD_EXT_CORE:
        case WPAD_EXT_NUNCHUK:
        case WPAD_EXT_CLASSIC:
        case WPAD_EXT_MPLUS:
        case WPAD_EXT_MPLUS_NUNCHUK:
        case WPAD_EXT_MPLUS_CLASSIC:
        case WPAD_EXT_PRO_CONTROLLER:
            return true;
        default:
            return false;
    }
}

std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>
LUSDeviceIndexMappingManager::CreateDeviceIndexMappingFromConfig(std::string id) {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(), "");

    if (mappingClass == "LUSDeviceIndexToWiiUDeviceIndexMapping") {
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        int32_t wiiuDeviceChannel =
            CVarGetInteger(StringHelper::Sprintf("%s.WiiUDeviceChannel", mappingCvarKey.c_str()).c_str(), -1);

        int32_t wiiuExtensionType =
            CVarGetInteger(StringHelper::Sprintf("%s.WiiUDeviceExtensionType", mappingCvarKey.c_str()).c_str(), -1);

        if (lusDeviceIndex < 0 || !IsValidWiiUExtensionType(wiiuExtensionType)) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<LUSDeviceIndexToSDLDeviceIndexMapping>(
            static_cast<LUSDeviceIndex>(lusDeviceIndex), sdlDeviceIndex, sdlJoystickGuid, sdlControllerName, stickAxisThreshold, triggerAxisThreshold);
    }

    return nullptr;
}

void LUSDeviceIndexMappingManager::InitializeMappingsSinglePlayer() {
}

void LUSDeviceIndexMappingManager::UpdateExtensionTypesFromConfig() {
    // todo: this efficently (when we build out cvar array support?)
    // i don't expect it to really be a problem with the small number of mappings we have
    // for each controller (especially compared to include/exclude locations in rando), and
    // the audio editor pattern doesn't work for this because that looks for ids that are either
    // hardcoded or provided by an otr file
    std::stringstream mappingIdsStringStream(CVarGetString("gControllers.DeviceMappingIds", ""));
    std::string mappingIdString;
    while (getline(mappingIdsStringStream, mappingIdString, ',')) {
        const std::string mappingCvarKey = "gControllers.DeviceMappings." + mappingIdString;
        const std::string mappingClass =
            CVarGetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(), "");

        if (mappingClass == "LUSDeviceIndexToWiiUDeviceIndexMapping") {
            mLUSDeviceIndexToWiiUDeviceTypes[static_cast<LUSDeviceIndex>(
                CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1))] =
                {
                    CVarGetInteger(StringHelper::Sprintf("%s.IsGamepad", mappingCvarKey.c_str()).c_str(), -1),
                    CVarGetInteger(StringHelper::Sprintf("%s.WiiUDeviceExtensionType", mappingCvarKey.c_str()).c_str(), -1)
                };
        }
    }
}

std::pair<bool, int32_t> LUSDeviceIndexMappingManager::GetWiiUDeviceTypeFromLUSDeviceIndex(LUSDeviceIndex index) {
    return mLUSDeviceIndexToWiiUDeviceTypes[index];
}
#else
void LUSDeviceIndexMappingManager::InitializeMappingsMultiplayer(std::vector<int32_t> sdlIndices) {
    for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
        for (auto mapping :
             Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex)->GetAllMappings()) {
            auto sdlMapping = std::dynamic_pointer_cast<SDLMapping>(mapping);
            if (sdlMapping == nullptr) {
                continue;
            }

            sdlMapping->CloseController();
        }
    }
    mLUSDeviceIndexToPhysicalDeviceIndexMappings.clear();
    uint8_t port = 0;
    for (auto sdlIndex : sdlIndices) {
        InitializeSDLMappingsForPort(port, sdlIndex);
        port++;
    }
    mIsInitialized = true;
}

void LUSDeviceIndexMappingManager::InitializeSDLMappingsForPort(uint8_t n64port, int32_t sdlIndex) {
    if (!SDL_IsGameController(sdlIndex)) {
        return;
    }

    char guidString[33]; // SDL_GUID_LENGTH + 1 for null terminator
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(sdlIndex), guidString, sizeof(guidString));
    std::string sdlControllerName = SDL_GameControllerNameForIndex(sdlIndex) != nullptr
                                        ? SDL_GameControllerNameForIndex(sdlIndex)
                                        : "Game Controller";

    // find all lus indices with this guid
    std::vector<LUSDeviceIndex> matchingGuidLusIndices;
    auto mappings = GetAllDeviceIndexMappingsFromConfig();
    for (auto [lusIndex, mapping] : mappings) {
        auto sdlMapping = std::dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(mapping);
        if (sdlMapping == nullptr) {
            continue;
        }

        if (sdlMapping->GetJoystickGUID() == guidString) {
            matchingGuidLusIndices.push_back(lusIndex);
        }
    }

    // set this device to the lowest available lus index with this guid
    for (auto lusIndex : matchingGuidLusIndices) {
        if (GetDeviceIndexMappingFromLUSDeviceIndex(lusIndex) != nullptr) {
            // we already loaded this one
            continue;
        }

        auto mapping = mappings[lusIndex];
        auto sdlMapping = std::dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(mapping);

        sdlMapping->SetSDLDeviceIndex(sdlIndex);
        SetLUSDeviceIndexToPhysicalDeviceIndexMapping(sdlMapping);

        // if we have mappings for this LUS device on this port, we're good and don't need to move any mappings
        if (Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->HasMappingsForLUSDeviceIndex(
                lusIndex)) {
            return;
        }

        // move mappings from other port to this one
        for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
            if (portIndex == n64port) {
                continue;
            }

            if (!Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex)->HasMappingsForLUSDeviceIndex(
                    lusIndex)) {
                continue;
            }

            Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex)->MoveMappingsToDifferentController(
                Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port), lusIndex);
            return;
        }

        // we shouldn't get here
        return;
    }

    // if we didn't find a mapping for this guid, make defaults
    auto lusIndex = GetLowestLUSDeviceIndexWithNoAssociatedButtonOrAxisDirectionMappings();
    auto deviceIndexMapping =
        std::make_shared<LUSDeviceIndexToSDLDeviceIndexMapping>(lusIndex, sdlIndex, guidString, sdlControllerName, 7680.0f, 7680.0f);
    mLUSDeviceIndexToSDLControllerNames[lusIndex] = sdlControllerName;
    deviceIndexMapping->SaveToConfig();
    SetLUSDeviceIndexToPhysicalDeviceIndexMapping(deviceIndexMapping);
    SaveMappingIdsToConfig();
    Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->AddDefaultMappings(lusIndex);
}

std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>
LUSDeviceIndexMappingManager::CreateDeviceIndexMappingFromConfig(std::string id) {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(), "");

    if (mappingClass == "LUSDeviceIndexToSDLDeviceIndexMapping") {
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        int32_t sdlDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        std::string sdlJoystickGuid =
            CVarGetString(StringHelper::Sprintf("%s.SDLJoystickGUID", mappingCvarKey.c_str()).c_str(), "");

        std::string sdlControllerName =
            CVarGetString(StringHelper::Sprintf("%s.SDLControllerName", mappingCvarKey.c_str()).c_str(), "");

        float stickAxisThreshold = CVarGetFloat(StringHelper::Sprintf("%s.StickAxisThreshold", mappingCvarKey.c_str()).c_str(), 7680.0f);
        float triggerAxisThreshold = CVarGetFloat(StringHelper::Sprintf("%s.TriggerAxisThreshold", mappingCvarKey.c_str()).c_str(), 7680.0f);

        if (lusDeviceIndex < 0 || sdlJoystickGuid == "") {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<LUSDeviceIndexToSDLDeviceIndexMapping>(
            static_cast<LUSDeviceIndex>(lusDeviceIndex), sdlDeviceIndex, sdlJoystickGuid, sdlControllerName, stickAxisThreshold, triggerAxisThreshold);
    }

    return nullptr;
}

void LUSDeviceIndexMappingManager::InitializeMappingsSinglePlayer() {
    // find all currently attached physical devices and map their guids to sdl index
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

    // prompt for use as saved vs use as new
    // if input mappings only exist for one LUS color, then use as saved doesn't need extra prompts
    // if input mappings exist for multiple LUS colors, then prompt for "which color"
    // is it possible to throw an imgui modal up with controller input temporarily enabled?
    // use as new set default mappings
    // only ask for use as saved if we don't have any "active" mappings on the port we're currently trying to map to
    // (start with 1, then 2 etc.)
    // - active defined as any non-keyboard input mapping on a port using an LUS index with a non-null
    // deviceindexmapping

    // prompt for what port
    // if all ports inactive, only one connected controller, no prompt for which port, just either apply existing or use
    // as new
    //

    // map any remaining controllers to the lowest available LUS index
    for (auto [sdlIndex, sdlGuid] : attachedSdlControllerGuids) {
        for (uint8_t lusIndex = LUSDeviceIndex::Blue; lusIndex < LUSDeviceIndex::Max; lusIndex++) {
            if (GetDeviceIndexMappingFromLUSDeviceIndex(static_cast<LUSDeviceIndex>(lusIndex)) != nullptr) {
                continue;
            }

            std::string sdlControllerName = SDL_GameControllerNameForIndex(sdlIndex) != nullptr
                                                ? SDL_GameControllerNameForIndex(sdlIndex)
                                                : "Game Controller";
            SetLUSDeviceIndexToPhysicalDeviceIndexMapping(std::make_shared<LUSDeviceIndexToSDLDeviceIndexMapping>(
                static_cast<LUSDeviceIndex>(lusIndex), sdlIndex, sdlGuid, sdlControllerName, 7680.0f, 7680.0f));
            mLUSDeviceIndexToSDLControllerNames[static_cast<LUSDeviceIndex>(lusIndex)] = sdlControllerName;
            break;
        }
    }

    for (auto [id, mapping] : mLUSDeviceIndexToPhysicalDeviceIndexMappings) {
        mapping->SaveToConfig();
    }
    SaveMappingIdsToConfig();
}

void LUSDeviceIndexMappingManager::UpdateControllerNamesFromConfig() {
    // todo: this efficently (when we build out cvar array support?)
    // i don't expect it to really be a problem with the small number of mappings we have
    // for each controller (especially compared to include/exclude locations in rando), and
    // the audio editor pattern doesn't work for this because that looks for ids that are either
    // hardcoded or provided by an otr file
    std::stringstream mappingIdsStringStream(CVarGetString("gControllers.DeviceMappingIds", ""));
    std::string mappingIdString;
    while (getline(mappingIdsStringStream, mappingIdString, ',')) {
        const std::string mappingCvarKey = "gControllers.DeviceMappings." + mappingIdString;
        const std::string mappingClass =
            CVarGetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(), "");

        if (mappingClass == "LUSDeviceIndexToSDLDeviceIndexMapping") {
            mLUSDeviceIndexToSDLControllerNames[static_cast<LUSDeviceIndex>(
                CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1))] =
                CVarGetString(StringHelper::Sprintf("%s.SDLControllerName", mappingCvarKey.c_str()).c_str(), "");
        }
    }
}

std::string LUSDeviceIndexMappingManager::GetSDLControllerNameFromLUSDeviceIndex(LUSDeviceIndex index) {
    return mLUSDeviceIndexToSDLControllerNames[index];
}

int32_t LUSDeviceIndexMappingManager::GetNewSDLDeviceIndexFromLUSDeviceIndex(LUSDeviceIndex lusIndex) {
    for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
        auto controller = Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex);

        for (auto [bitmask, button] : controller->GetAllButtons()) {
            for (auto [id, buttonMapping] : button->GetAllButtonMappings()) {
                if (buttonMapping->GetLUSDeviceIndex() != lusIndex) {
                    continue;
                }

                auto sdlButtonMapping = std::dynamic_pointer_cast<SDLMapping>(buttonMapping);
                if (sdlButtonMapping == nullptr) {
                    continue;
                }

                return sdlButtonMapping->GetCurrentSDLDeviceIndex();
            }
        }

        for (auto stick : { controller->GetLeftStick(), controller->GetRightStick() }) {
            for (auto [direction, axisDirectionMappings] : stick->GetAllAxisDirectionMappings()) {
                for (auto [id, axisDirectionMapping] : axisDirectionMappings) {
                    if (axisDirectionMapping->GetLUSDeviceIndex() != lusIndex) {
                        continue;
                    }

                    auto sdlAxisDirectionMapping = std::dynamic_pointer_cast<SDLMapping>(axisDirectionMapping);
                    if (sdlAxisDirectionMapping == nullptr) {
                        continue;
                    }

                    return sdlAxisDirectionMapping->GetCurrentSDLDeviceIndex();
                }
            }
        }

        auto sdlGyroMapping = std::dynamic_pointer_cast<SDLMapping>(controller->GetGyro()->GetGyroMapping());
        if (sdlGyroMapping != nullptr && sdlGyroMapping->GetLUSDeviceIndex() == lusIndex) {
            return sdlGyroMapping->GetCurrentSDLDeviceIndex();
        }

        for (auto [id, rumbleMapping] : controller->GetRumble()->GetAllRumbleMappings()) {
            if (rumbleMapping->GetLUSDeviceIndex() != lusIndex) {
                continue;
            }

            auto sdlRumbleMapping = std::dynamic_pointer_cast<SDLMapping>(rumbleMapping);
            if (sdlRumbleMapping == nullptr) {
                continue;
            }

            return sdlRumbleMapping->GetCurrentSDLDeviceIndex();
        }

        for (auto [id, ledMapping] : controller->GetLED()->GetAllLEDMappings()) {
            if (ledMapping->GetLUSDeviceIndex() != lusIndex) {
                continue;
            }

            auto sdlLEDMapping = std::dynamic_pointer_cast<SDLMapping>(ledMapping);
            if (sdlLEDMapping == nullptr) {
                continue;
            }

            return sdlLEDMapping->GetCurrentSDLDeviceIndex();
        }
    }

    // couldn't find one
    return -1;
}

void LUSDeviceIndexMappingManager::HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex) {
    if (!mIsInitialized) {
        return;
    }

    if (!SDL_IsGameController(sdlDeviceIndex)) {
        return;
    }

    auto controllerDisconnectedWindow = std::dynamic_pointer_cast<ControllerDisconnectedWindow>(
        Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Controller Disconnected"));
    if (controllerDisconnectedWindow != nullptr && controllerDisconnectedWindow->IsVisible()) {
        // don't try to automap if we are looking at the controller disconnected modal
        return;
    }

    if (Context::GetInstance()->GetControlDeck()->IsSinglePlayerMappingMode()) {
        // todo: handle single player mode
    } else {
        for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
            bool portInUse = false;
            for (auto mapping :
                 Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex)->GetAllMappings()) {
                auto sdlMapping = std::dynamic_pointer_cast<SDLMapping>(mapping);
                if (sdlMapping == nullptr) {
                    continue;
                }

                if (sdlMapping->ControllerLoaded()) {
                    portInUse = true;
                    break;
                }
            }

            if (portInUse) {
                continue;
            }

            InitializeSDLMappingsForPort(portIndex, sdlDeviceIndex);
            return;
        }
    }
}

void LUSDeviceIndexMappingManager::HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId) {
    auto lusIndexOfPhysicalDeviceThatHasBeenDisconnected =
        GetLUSDeviceIndexOfDisconnectedPhysicalDevice(sdlJoystickInstanceId);

    if (lusIndexOfPhysicalDeviceThatHasBeenDisconnected == LUSDeviceIndex::Max) {
        // for some reason we don't know what device was disconnected, prompt to reorder
        auto controllerDisconnectedWindow = std::dynamic_pointer_cast<ControllerDisconnectedWindow>(
            Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Controller Disconnected"));
        if (controllerDisconnectedWindow != nullptr) {
            // todo: don't use UINT8_MAX-1 to mean we don't know what controller was disconnected
            controllerDisconnectedWindow->SetPortIndexOfDisconnectedController(UINT8_MAX - 1);
            controllerDisconnectedWindow->Show();
        } else {
            // todo: log error
        }
        return;
    }

    for (auto [lusIndex, mapping] : mLUSDeviceIndexToPhysicalDeviceIndexMappings) {
        auto sdlMapping = dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(mapping);
        if (sdlMapping == nullptr) {
            continue;
        }

        if (lusIndex == lusIndexOfPhysicalDeviceThatHasBeenDisconnected) {
            sdlMapping->SetSDLDeviceIndex(-1);
            sdlMapping->SaveToConfig();
            auto controllerDisconnectedWindow = std::dynamic_pointer_cast<ControllerDisconnectedWindow>(
                Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Controller Disconnected"));
            if (controllerDisconnectedWindow != nullptr) {
                controllerDisconnectedWindow->SetPortIndexOfDisconnectedController(
                    GetPortIndexOfDisconnectedPhysicalDevice(sdlJoystickInstanceId));
                controllerDisconnectedWindow->Show();
            }
            continue;
        }

        sdlMapping->SetSDLDeviceIndex(GetNewSDLDeviceIndexFromLUSDeviceIndex(lusIndex));
        sdlMapping->SaveToConfig();
    }

    RemoveLUSDeviceIndexToPhysicalDeviceIndexMapping(lusIndexOfPhysicalDeviceThatHasBeenDisconnected);
}

LUSDeviceIndex LUSDeviceIndexMappingManager::GetLUSDeviceIndexFromSDLDeviceIndex(int32_t sdlIndex) {
    for (auto [lusIndex, mapping] : mLUSDeviceIndexToPhysicalDeviceIndexMappings) {
        auto sdlMapping = dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(mapping);
        if (sdlMapping == nullptr) {
            continue;
        }

        if (sdlMapping->GetSDLDeviceIndex() == sdlIndex) {
            return lusIndex;
        }
    }

    // didn't find one
    return LUSDeviceIndex::Max;
}

uint8_t LUSDeviceIndexMappingManager::GetPortIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId) {
    for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
        auto controller = Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex);

        for (auto [bitmask, button] : controller->GetAllButtons()) {
            for (auto [id, buttonMapping] : button->GetAllButtonMappings()) {
                auto sdlButtonMapping = std::dynamic_pointer_cast<SDLMapping>(buttonMapping);
                if (sdlButtonMapping == nullptr) {
                    continue;
                }
                if (sdlButtonMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
                    return portIndex;
                }
            }
        }

        for (auto stick : { controller->GetLeftStick(), controller->GetRightStick() }) {
            for (auto [direction, axisDirectionMappings] : stick->GetAllAxisDirectionMappings()) {
                for (auto [id, axisDirectionMapping] : axisDirectionMappings) {
                    auto sdlAxisDirectionMapping = std::dynamic_pointer_cast<SDLMapping>(axisDirectionMapping);
                    if (sdlAxisDirectionMapping == nullptr) {
                        continue;
                    }
                    if (sdlAxisDirectionMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
                        return portIndex;
                    }
                }
            }
        }

        auto sdlGyroMapping = std::dynamic_pointer_cast<SDLMapping>(controller->GetGyro()->GetGyroMapping());
        if (sdlGyroMapping != nullptr && sdlGyroMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
            return portIndex;
        }

        for (auto [id, rumbleMapping] : controller->GetRumble()->GetAllRumbleMappings()) {
            auto sdlRumbleMapping = std::dynamic_pointer_cast<SDLMapping>(rumbleMapping);
            if (sdlRumbleMapping == nullptr) {
                continue;
            }
            if (sdlRumbleMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
                return portIndex;
            }
        }

        for (auto [id, ledMapping] : controller->GetLED()->GetAllLEDMappings()) {
            auto sdlLEDMapping = std::dynamic_pointer_cast<SDLMapping>(ledMapping);
            if (sdlLEDMapping == nullptr) {
                continue;
            }
            if (sdlLEDMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
                return portIndex;
            }
        }
    }

    // couldn't find one
    return UINT8_MAX;
}

LUSDeviceIndex
LUSDeviceIndexMappingManager::GetLUSDeviceIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId) {
    for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
        auto controller = Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex);

        for (auto [bitmask, button] : controller->GetAllButtons()) {
            for (auto [id, buttonMapping] : button->GetAllButtonMappings()) {
                auto sdlButtonMapping = std::dynamic_pointer_cast<SDLMapping>(buttonMapping);
                if (sdlButtonMapping == nullptr) {
                    continue;
                }
                if (sdlButtonMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
                    return sdlButtonMapping->GetLUSDeviceIndex();
                }
            }
        }

        for (auto stick : { controller->GetLeftStick(), controller->GetRightStick() }) {
            for (auto [direction, axisDirectionMappings] : stick->GetAllAxisDirectionMappings()) {
                for (auto [id, axisDirectionMapping] : axisDirectionMappings) {
                    auto sdlAxisDirectionMapping = std::dynamic_pointer_cast<SDLMapping>(axisDirectionMapping);
                    if (sdlAxisDirectionMapping == nullptr) {
                        continue;
                    }
                    if (sdlAxisDirectionMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
                        return sdlAxisDirectionMapping->GetLUSDeviceIndex();
                    }
                }
            }
        }

        auto sdlGyroMapping = std::dynamic_pointer_cast<SDLMapping>(controller->GetGyro()->GetGyroMapping());
        if (sdlGyroMapping != nullptr && sdlGyroMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
            return sdlGyroMapping->GetLUSDeviceIndex();
        }

        for (auto [id, rumbleMapping] : controller->GetRumble()->GetAllRumbleMappings()) {
            auto sdlRumbleMapping = std::dynamic_pointer_cast<SDLMapping>(rumbleMapping);
            if (sdlRumbleMapping == nullptr) {
                continue;
            }
            if (sdlRumbleMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
                return sdlRumbleMapping->GetLUSDeviceIndex();
            }
        }

        for (auto [id, ledMapping] : controller->GetLED()->GetAllLEDMappings()) {
            auto sdlLEDMapping = std::dynamic_pointer_cast<SDLMapping>(ledMapping);
            if (sdlLEDMapping == nullptr) {
                continue;
            }
            if (sdlLEDMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
                return sdlLEDMapping->GetLUSDeviceIndex();
            }
        }
    }

    // couldn't find one
    return LUSDeviceIndex::Max;
}
#endif

LUSDeviceIndex LUSDeviceIndexMappingManager::GetLowestLUSDeviceIndexWithNoAssociatedButtonOrAxisDirectionMappings() {
    for (uint8_t lusIndex = LUSDeviceIndex::Blue; lusIndex < LUSDeviceIndex::Max; lusIndex++) {
        if (Context::GetInstance()->GetControlDeck()->GetControllerByPort(0)->HasMappingsForLUSDeviceIndex(
                static_cast<LUSDeviceIndex>(lusIndex)) ||
            Context::GetInstance()->GetControlDeck()->GetControllerByPort(1)->HasMappingsForLUSDeviceIndex(
                static_cast<LUSDeviceIndex>(lusIndex)) ||
            Context::GetInstance()->GetControlDeck()->GetControllerByPort(2)->HasMappingsForLUSDeviceIndex(
                static_cast<LUSDeviceIndex>(lusIndex)) ||
            Context::GetInstance()->GetControlDeck()->GetControllerByPort(3)->HasMappingsForLUSDeviceIndex(
                static_cast<LUSDeviceIndex>(lusIndex))) {
            continue;
        }
        return static_cast<LUSDeviceIndex>(lusIndex);
    }

    // todo: invalid?
    return LUSDeviceIndex::Max;
}

void LUSDeviceIndexMappingManager::SaveMappingIdsToConfig() {
    // todo: this efficently (when we build out cvar array support?)

    std::set<std::string> ids;
    std::stringstream mappingIdsStringStream(CVarGetString("gControllers.DeviceMappingIds", ""));
    std::string mappingIdString;
    while (getline(mappingIdsStringStream, mappingIdString, ',')) {
        ids.insert(mappingIdString);
    }

    for (auto [lusIndex, mapping] : mLUSDeviceIndexToPhysicalDeviceIndexMappings) {
        ids.insert(mapping->GetMappingId());
    }

    std::string mappingIdListString = "";
    for (auto id : ids) {
        mappingIdListString += id;
        mappingIdListString += ",";
    }

    if (mappingIdListString == "") {
        CVarClear("gControllers.DeviceMappingIds");
    } else {
        CVarSetString("gControllers.DeviceMappingIds", mappingIdListString.c_str());
    }

    CVarSave();
}

std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>>
LUSDeviceIndexMappingManager::GetAllDeviceIndexMappingsFromConfig() {
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

std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>
LUSDeviceIndexMappingManager::GetDeviceIndexMappingFromLUSDeviceIndex(LUSDeviceIndex lusIndex) {
    if (!mLUSDeviceIndexToPhysicalDeviceIndexMappings.contains(lusIndex)) {
        return nullptr;
    }

    return mLUSDeviceIndexToPhysicalDeviceIndexMappings[lusIndex];
}

std::unordered_map<LUSDeviceIndex, std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping>>
LUSDeviceIndexMappingManager::GetAllDeviceIndexMappings() {
    return mLUSDeviceIndexToPhysicalDeviceIndexMappings;
}

void LUSDeviceIndexMappingManager::SetLUSDeviceIndexToPhysicalDeviceIndexMapping(
    std::shared_ptr<LUSDeviceIndexToPhysicalDeviceIndexMapping> mapping) {
    mLUSDeviceIndexToPhysicalDeviceIndexMappings[mapping->GetLUSDeviceIndex()] = mapping;
}

void LUSDeviceIndexMappingManager::RemoveLUSDeviceIndexToPhysicalDeviceIndexMapping(LUSDeviceIndex index) {
    mLUSDeviceIndexToPhysicalDeviceIndexMappings.erase(index);
}
} // namespace LUS
