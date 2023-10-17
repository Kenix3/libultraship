#include "LUSDeviceIndexMappingManager.h"
#include <SDL2/SDL.h>
#include <Utils/StringHelper.h>
#include "public/bridge/consolevariablebridge.h"
#include <vector>
#include "Context.h"
#include "ControllerDisconnectedWindow.h"
#include "ControllerReorderingWindow.h"
#include "controller/controldevice/controller/mapping/sdl/SDLMapping.h"
#include <sstream>

namespace LUS {
LUSDeviceIndexMappingManager::LUSDeviceIndexMappingManager() : mIsInitialized(false) {
}

LUSDeviceIndexMappingManager::~LUSDeviceIndexMappingManager() {
}

void LUSDeviceIndexMappingManager::InitializeMappingsMultiplayer(std::vector<int32_t> sdlIndices) {
    mLUSDeviceIndexToPhysicalDeviceIndexMappings.clear();
    uint8_t port = 0;
    for (auto sdlIndex : sdlIndices) {
        InitializeSDLMappingsForPort(port, sdlIndex);
        port++;
    }
    mIsInitialized = true;
}

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

void LUSDeviceIndexMappingManager::InitializeSDLMappingsForPort(uint8_t n64port, int32_t sdlIndex) {
    if (!SDL_IsGameController(sdlIndex)) {
        return;
    }

    char guidString[33]; // SDL_GUID_LENGTH + 1 for null terminator
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(sdlIndex), guidString, sizeof(guidString));

    // find all lus indices with this guid
    std::map<int32_t, LUSDeviceIndex> matchingGuidLusIndices;
    auto mappings = GetAllDeviceIndexMappingsFromConfig();
    for (auto [lusIndex, mapping] : mappings) {
        auto sdlMapping = std::dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(mapping);
        if (sdlMapping == nullptr) {
            continue;
        }

        if (sdlMapping->GetJoystickGUID() == guidString) {
            matchingGuidLusIndices[sdlMapping->GetSDLDeviceIndex()] = lusIndex;
        }
    }

    // set this device to the lowest available lus index with this guid
    for (auto [sdlIndexFromConfig, lusIndex] : matchingGuidLusIndices) {
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
    auto deviceIndexMapping = std::make_shared<LUSDeviceIndexToSDLDeviceIndexMapping>(lusIndex, sdlIndex, guidString);
    deviceIndexMapping->SaveToConfig();
    SetLUSDeviceIndexToPhysicalDeviceIndexMapping(deviceIndexMapping);
    SaveMappingIdsToConfig();
    Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->AddDefaultMappings(lusIndex);
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

            SetLUSDeviceIndexToPhysicalDeviceIndexMapping(std::make_shared<LUSDeviceIndexToSDLDeviceIndexMapping>(
                static_cast<LUSDeviceIndex>(lusIndex), sdlIndex, sdlGuid));
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

        if (lusDeviceIndex < 0 || sdlDeviceIndex < 0 || sdlJoystickGuid == "") {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<LUSDeviceIndexToSDLDeviceIndexMapping>(static_cast<LUSDeviceIndex>(lusDeviceIndex),
                                                                       sdlDeviceIndex, sdlJoystickGuid);
    }

    return nullptr;
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

    // todo: things when connecting new controllers without disconnect stuff
}

void LUSDeviceIndexMappingManager::HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId) {
    auto lusIndexOfPhysicalDeviceThatHasBeenDisconnected =
        GetLUSDeviceIndexOfDisconnectedPhysicalDevice(sdlJoystickInstanceId);

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

/*

more todo space

"single player" mode

use case: 4 xbox controllers
- first boot (empty json)
  - default mappings for all 4 controllers appear in port 1 of the controller config menu
- quit/restart
  - with 4 xbox controllers connected (same sdl indices)
    - controllers reassociate with lus device index based on guid/sdl index
  - with sdl0 as switch controller, sdl1 as playstation controller, and 4 xbox controllers connected (sdl2-sdl5)
    - 2 xbox controllers reassociate with lus device index based on guid/sdl index
    - 2 xbox controllers reassociate with lus device index based on guid alone (lowest sdl index to lowest lus index)
    - new default mappings added to port 1 for switch and playstation controllers
  - with 1 xbox controller
    - controller reassociates based on guid/sdl index if possible, if sdl id doesn't match it associates with guid alone
      - i was worried we might have a risk of nonunique mappings here, but if we always match on both sdl index and guid
first then even if we update the sdl index when reassociating on guid alone we'll be fine
- new controller connected
  - try to associate based on guid/sdl index, if we can't try to associate based on guid, if we can't add default
mappings to port 1
- controller disconnected
  - can we use SDL_GameControllerGetAttached to figure out which controller was disconnected?
  - might be able to do something with deviceinstanceid comparisons, use SDL_JoystickGetDeviceInstanceID(device_index)
and SDL_JoystickInstanceID(joystick)
  - SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller))
  - we have no idea what controller the player is currently using, but it would be annoying to pause gameplay if an
unused controller got disconnected, so i think it makes sense to just show a little alert saying "controllername
disconnected"
  - automatically update all lusdeviceindex mappings to have the appropriate sdl device index
*/