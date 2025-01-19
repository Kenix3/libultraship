#include "ShipDeviceIndexMappingManager.h"
#include <SDL2/SDL.h>
#include "utils/StringHelper.h"
#include "public/bridge/consolevariablebridge.h"
#include <vector>
#include "Context.h"
#include "ControllerDisconnectedWindow.h"
#include "ControllerReorderingWindow.h"

#include "controller/controldevice/controller/mapping/sdl/SDLMapping.h"

#include <sstream>

namespace Ship {
ShipDeviceIndexMappingManager::ShipDeviceIndexMappingManager() : mIsInitialized(false) {
#if _WIN32
    SDL_SetHintWithPriority(SDL_HINT_JOYSTICK_WGI, "0", SDL_HINT_DEFAULT);
#endif
    UpdateControllerNamesFromConfig();
}

ShipDeviceIndexMappingManager::~ShipDeviceIndexMappingManager() {
}

void ShipDeviceIndexMappingManager::InitializeSDLMappingsForPort(uint8_t n64port) {
    // char guidString[33]; // SDL_GUID_LENGTH + 1 for null terminator
    // SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(sdlIndex), guidString, sizeof(guidString));
    // std::string sdlControllerName = SDL_GameControllerNameForIndex(sdlIndex) != nullptr
    //                                     ? SDL_GameControllerNameForIndex(sdlIndex)
    //                                     : "Game Controller";

    // // find all lus indices with this guid
    // std::vector<PhysicalDeviceType> matchingGuidLusIndices;
    // auto mappings = GetAllDeviceIndexMappingsFromConfig();
    // for (auto [lusIndex, mapping] : mappings) {
    //     auto sdlMapping = std::dynamic_pointer_cast<ShipDeviceIndexToSDLDeviceIndexMapping>(mapping);
    //     if (sdlMapping == nullptr) {
    //         continue;
    //     }

    //     if (sdlMapping->GetJoystickGUID() == guidString) {
    //         matchingGuidLusIndices.push_back(lusIndex);
    //     }
    // }

    // // set this device to the lowest available lus index with this guid
    // for (auto lusIndex : matchingGuidLusIndices) {
    //     if (GetDeviceIndexMappingFromShipDeviceIndex(lusIndex) != nullptr) {
    //         // we already loaded this one
    //         continue;
    //     }

    //     auto mapping = mappings[lusIndex];
    //     auto sdlMapping = std::dynamic_pointer_cast<ShipDeviceIndexToSDLDeviceIndexMapping>(mapping);

    //     sdlMapping->SetSDLDeviceIndex(sdlIndex);
    //     SetShipDeviceIndexToPhysicalDeviceIndexMapping(sdlMapping);

    //     // if we have mappings for this LUS device on this port, we're good and don't need to move any mappings
    //     if (Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->HasMappingsForPhysicalDeviceType(
    //             lusIndex)) {
    //         return;
    //     }

    //     if (!Context::GetInstance()->GetControlDeck()->IsSinglePlayerMappingMode()) {
    //         // move mappings from other port to this one
    //         for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
    //             if (portIndex == n64port) {
    //                 continue;
    //             }

    //             if (!Context::GetInstance()
    //                      ->GetControlDeck()
    //                      ->GetControllerByPort(portIndex)
    //                      ->HasMappingsForPhysicalDeviceType(lusIndex)) {
    //                 continue;
    //             }

    //             Context::GetInstance()
    //                 ->GetControlDeck()
    //                 ->GetControllerByPort(portIndex)
    //                 ->MoveMappingsToDifferentController(
    //                     Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port), lusIndex);
    //             return;
    //         }
    //     }

    //     // we have a device index mapping but no button/axis/etc. mappings, make defaults
    //     Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->AddDefaultMappings(lusIndex);
    //     return;
    // }

    // // if we didn't find a mapping for this guid, make defaults
    // auto lusIndex = GetLowestShipDeviceIndexWithNoAssociatedButtonOrAxisDirectionMappings();
    // auto deviceIndexMapping = std::make_shared<ShipDeviceIndexToSDLDeviceIndexMapping>(lusIndex, sdlIndex,
    // guidString,
    //                                                                                    sdlControllerName, 25, 25);
    // mShipDeviceIndexToSDLControllerNames[lusIndex] = sdlControllerName;
    // deviceIndexMapping->SaveToConfig();
    // SetShipDeviceIndexToPhysicalDeviceIndexMapping(deviceIndexMapping);
    // SaveMappingIdsToConfig();
    // Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->AddDefaultMappings(lusIndex);
}

std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>
ShipDeviceIndexMappingManager::CreateDeviceIndexMappingFromConfig(std::string id) {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".DeviceMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(), "");

    if (mappingClass == "ShipDeviceIndexToSDLDeviceIndexMapping") {
        int32_t shipDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        int32_t sdlDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        std::string sdlJoystickGuid =
            CVarGetString(StringHelper::Sprintf("%s.SDLJoystickGUID", mappingCvarKey.c_str()).c_str(), "");

        std::string sdlControllerName =
            CVarGetString(StringHelper::Sprintf("%s.SDLControllerName", mappingCvarKey.c_str()).c_str(), "");

        int32_t stickAxisThreshold = CVarGetInteger(
            StringHelper::Sprintf("%s.StickAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(), 25);
        int32_t triggerAxisThreshold = CVarGetInteger(
            StringHelper::Sprintf("%s.TriggerAxisThresholdPercentage", mappingCvarKey.c_str()).c_str(), 25);

        if (shipDeviceIndex < 0 || sdlJoystickGuid == "") {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<ShipDeviceIndexToSDLDeviceIndexMapping>(
            static_cast<PhysicalDeviceType>(shipDeviceIndex), sdlDeviceIndex, sdlJoystickGuid, sdlControllerName,
            stickAxisThreshold, triggerAxisThreshold);
    }

    return nullptr;
}

void ShipDeviceIndexMappingManager::InitializeMappingsSinglePlayer() {
    // for (auto mapping : Context::GetInstance()->GetControlDeck()->GetControllerByPort(0)->GetAllMappings()) {
    //     auto sdlMapping = std::dynamic_pointer_cast<SDLMapping>(mapping);
    //     if (sdlMapping == nullptr) {
    //         continue;
    //     }

    //     sdlMapping->CloseController();
    // }

    // std::vector<int32_t> connectedSdlControllerIndices;
    // for (auto i = 0; i < SDL_NumJoysticks(); i++) {
    //     if (SDL_IsGameController(i)) {
    //         connectedSdlControllerIndices.push_back(i);
    //     }
    // }

    // mShipDeviceIndexToPhysicalDeviceIndexMappings.clear();
    // for (auto sdlIndex : connectedSdlControllerIndices) {
    InitializeSDLMappingsForPort(0);
    // }
    // mIsInitialized = true;
}

void ShipDeviceIndexMappingManager::UpdateControllerNamesFromConfig() {
    // todo: this efficently (when we build out cvar array support?)
    // i don't expect it to really be a problem with the small number of mappings we have
    // for each controller (especially compared to include/exclude locations in rando), and
    // the audio editor pattern doesn't work for this because that looks for ids that are either
    // hardcoded or provided by an otr file
    std::stringstream mappingIdsStringStream(CVarGetString(CVAR_PREFIX_CONTROLLERS ".DeviceMappingIds", ""));
    std::string mappingIdString;
    while (getline(mappingIdsStringStream, mappingIdString, ',')) {
        const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".DeviceMappings." + mappingIdString;
        const std::string mappingClass =
            CVarGetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(), "");

        if (mappingClass == "ShipDeviceIndexToSDLDeviceIndexMapping") {
            mShipDeviceIndexToSDLControllerNames[static_cast<PhysicalDeviceType>(
                CVarGetInteger(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str(), -1))] =
                CVarGetString(StringHelper::Sprintf("%s.SDLControllerName", mappingCvarKey.c_str()).c_str(), "");
        }
    }
}

std::string ShipDeviceIndexMappingManager::GetSDLControllerNameFromShipDeviceIndex(PhysicalDeviceType index) {
    return mShipDeviceIndexToSDLControllerNames[index];
}

int32_t ShipDeviceIndexMappingManager::GetNewSDLDeviceIndexFromShipDeviceIndex(PhysicalDeviceType lusIndex) {
    // for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
    //     auto controller = Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex);

    //     for (auto [bitmask, button] : controller->GetAllButtons()) {
    //         for (auto [id, buttonMapping] : button->GetAllButtonMappings()) {
    //             if (buttonMapping->GetPhysicalDeviceType() != lusIndex) {
    //                 continue;
    //             }

    //             auto sdlButtonMapping = std::dynamic_pointer_cast<SDLMapping>(buttonMapping);
    //             if (sdlButtonMapping == nullptr) {
    //                 continue;
    //             }

    //             return sdlButtonMapping->GetCurrentSDLDeviceIndex();
    //         }
    //     }

    //     for (auto stick : { controller->GetLeftStick(), controller->GetRightStick() }) {
    //         for (auto [direction, axisDirectionMappings] : stick->GetAllAxisDirectionMappings()) {
    //             for (auto [id, axisDirectionMapping] : axisDirectionMappings) {
    //                 if (axisDirectionMapping->GetPhysicalDeviceType() != lusIndex) {
    //                     continue;
    //                 }

    //                 auto sdlAxisDirectionMapping = std::dynamic_pointer_cast<SDLMapping>(axisDirectionMapping);
    //                 if (sdlAxisDirectionMapping == nullptr) {
    //                     continue;
    //                 }

    //                 return sdlAxisDirectionMapping->GetCurrentSDLDeviceIndex();
    //             }
    //         }
    //     }

    //     auto sdlGyroMapping = std::dynamic_pointer_cast<SDLMapping>(controller->GetGyro()->GetGyroMapping());
    //     if (sdlGyroMapping != nullptr && sdlGyroMapping->GetPhysicalDeviceType() == lusIndex) {
    //         return sdlGyroMapping->GetCurrentSDLDeviceIndex();
    //     }

    //     for (auto [id, rumbleMapping] : controller->GetRumble()->GetAllRumbleMappings()) {
    //         if (rumbleMapping->GetPhysicalDeviceType() != lusIndex) {
    //             continue;
    //         }

    //         auto sdlRumbleMapping = std::dynamic_pointer_cast<SDLMapping>(rumbleMapping);
    //         if (sdlRumbleMapping == nullptr) {
    //             continue;
    //         }

    //         return sdlRumbleMapping->GetCurrentSDLDeviceIndex();
    //     }

    //     for (auto [id, ledMapping] : controller->GetLED()->GetAllLEDMappings()) {
    //         if (ledMapping->GetPhysicalDeviceType() != lusIndex) {
    //             continue;
    //         }

    //         auto sdlLEDMapping = std::dynamic_pointer_cast<SDLMapping>(ledMapping);
    //         if (sdlLEDMapping == nullptr) {
    //             continue;
    //         }

    //         return sdlLEDMapping->GetCurrentSDLDeviceIndex();
    //     }
    // }

    // couldn't find one
    return -1;
}

void ShipDeviceIndexMappingManager::HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex) {
    // if (!mIsInitialized) {
    //     return;
    // }

    // if (!SDL_IsGameController(sdlDeviceIndex)) {
    //     return;
    // }

    // auto controllerDisconnectedWindow = std::dynamic_pointer_cast<ControllerDisconnectedWindow>(
    //     Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Controller Disconnected"));
    // if (controllerDisconnectedWindow != nullptr && controllerDisconnectedWindow->IsVisible()) {
    //     // don't try to automap if we are looking at the controller disconnected modal
    //     return;
    // }

    // if (Context::GetInstance()->GetControlDeck()->IsSinglePlayerMappingMode()) {
    //     std::set<PhysicalDeviceType> alreadyConnectedDevices;
    //     for (auto mapping : Context::GetInstance()->GetControlDeck()->GetControllerByPort(0)->GetAllMappings()) {
    //         auto sdlMapping = std::dynamic_pointer_cast<SDLMapping>(mapping);
    //         if (sdlMapping == nullptr) {
    //             continue;
    //         }

    //         if (sdlMapping->ControllerLoaded()) {
    //             alreadyConnectedDevices.insert(sdlMapping->GetPhysicalDeviceType());
    //         }
    //     }

    //     for (auto [lusIndex, mapping] : mShipDeviceIndexToPhysicalDeviceIndexMappings) {
    //         auto sdlMapping = dynamic_pointer_cast<ShipDeviceIndexToSDLDeviceIndexMapping>(mapping);
    //         if (sdlMapping == nullptr) {
    //             continue;
    //         }

    //         if (alreadyConnectedDevices.contains(lusIndex)) {
    //             sdlMapping->SetSDLDeviceIndex(GetNewSDLDeviceIndexFromShipDeviceIndex(lusIndex));
    //             sdlMapping->SaveToConfig();
    //         }
    //     }

    //     InitializeSDLMappingsForPort(0, sdlDeviceIndex);
    //     return;
    // } else {
    //     for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
    //         bool portInUse = false;
    //         for (auto mapping :
    //              Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex)->GetAllMappings()) {
    //             auto sdlMapping = std::dynamic_pointer_cast<SDLMapping>(mapping);
    //             if (sdlMapping == nullptr) {
    //                 continue;
    //             }

    //             if (sdlMapping->ControllerLoaded()) {
    //                 portInUse = true;
    //                 break;
    //             }
    //         }

    //         if (portInUse) {
    //             continue;
    //         }

    //         InitializeSDLMappingsForPort(portIndex, sdlDeviceIndex);
    //         return;
    //     }
    // }
}

void ShipDeviceIndexMappingManager::HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId) {
    if (Context::GetInstance()->GetControlDeck()->IsSinglePlayerMappingMode()) {
        HandlePhysicalDeviceDisconnectSinglePlayer(sdlJoystickInstanceId);
    } else {
        HandlePhysicalDeviceDisconnectMultiplayer(sdlJoystickInstanceId);
    }
}

void ShipDeviceIndexMappingManager::HandlePhysicalDeviceDisconnectSinglePlayer(int32_t sdlJoystickInstanceId) {
    auto lusIndexOfPhysicalDeviceThatHasBeenDisconnected =
        GetShipDeviceIndexOfDisconnectedPhysicalDevice(sdlJoystickInstanceId);

    if (lusIndexOfPhysicalDeviceThatHasBeenDisconnected == PhysicalDeviceType::Max) {
        // for some reason we don't know what device was disconnected
        Context::GetInstance()->GetWindow()->GetGui()->GetGameOverlay()->TextDrawNotification(
            5, true, "Unknown device disconnected");
        InitializeMappingsSinglePlayer();
        return;
    }

    for (auto [lusIndex, mapping] : mShipDeviceIndexToPhysicalDeviceIndexMappings) {
        auto sdlMapping = dynamic_pointer_cast<ShipDeviceIndexToSDLDeviceIndexMapping>(mapping);
        if (sdlMapping == nullptr) {
            continue;
        }

        if (lusIndex == lusIndexOfPhysicalDeviceThatHasBeenDisconnected) {
            sdlMapping->SetSDLDeviceIndex(-1);
            sdlMapping->SaveToConfig();
            continue;
        }

        sdlMapping->SetSDLDeviceIndex(GetNewSDLDeviceIndexFromShipDeviceIndex(lusIndex));
        sdlMapping->SaveToConfig();
    }

    if (Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetAllDeviceIndexMappingsFromConfig()
            .count(lusIndexOfPhysicalDeviceThatHasBeenDisconnected) > 0) {
        auto deviceMapping =
            Context::GetInstance()
                ->GetControlDeck()
                ->GetDeviceIndexMappingManager()
                ->GetAllDeviceIndexMappingsFromConfig()[lusIndexOfPhysicalDeviceThatHasBeenDisconnected];
        auto sdlIndexMapping = std::static_pointer_cast<ShipDeviceIndexToSDLDeviceIndexMapping>(deviceMapping);
        if (sdlIndexMapping != nullptr) {
            Context::GetInstance()->GetWindow()->GetGui()->GetGameOverlay()->TextDrawNotification(
                5, true, "%s disconnected", sdlIndexMapping->GetSDLControllerName().c_str());
        }
    }

    RemoveShipDeviceIndexToPhysicalDeviceIndexMapping(lusIndexOfPhysicalDeviceThatHasBeenDisconnected);
}

void ShipDeviceIndexMappingManager::HandlePhysicalDeviceDisconnectMultiplayer(int32_t sdlJoystickInstanceId) {
    auto portIndexOfPhysicalDeviceThatHasBeenDisconnected =
        GetPortIndexOfDisconnectedPhysicalDevice(sdlJoystickInstanceId);
    auto lusIndexOfPhysicalDeviceThatHasBeenDisconnected =
        GetShipDeviceIndexOfDisconnectedPhysicalDevice(sdlJoystickInstanceId);

    if (lusIndexOfPhysicalDeviceThatHasBeenDisconnected == PhysicalDeviceType::Max) {
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

    for (auto [lusIndex, mapping] : mShipDeviceIndexToPhysicalDeviceIndexMappings) {
        auto sdlMapping = dynamic_pointer_cast<ShipDeviceIndexToSDLDeviceIndexMapping>(mapping);
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
                    portIndexOfPhysicalDeviceThatHasBeenDisconnected);
                controllerDisconnectedWindow->Show();
            }
            continue;
        }

        sdlMapping->SetSDLDeviceIndex(GetNewSDLDeviceIndexFromShipDeviceIndex(lusIndex));
        sdlMapping->SaveToConfig();
    }

    RemoveShipDeviceIndexToPhysicalDeviceIndexMapping(lusIndexOfPhysicalDeviceThatHasBeenDisconnected);
}

PhysicalDeviceType ShipDeviceIndexMappingManager::GetShipDeviceIndexFromSDLDeviceIndex(int32_t sdlIndex) {
    for (auto [lusIndex, mapping] : mShipDeviceIndexToPhysicalDeviceIndexMappings) {
        auto sdlMapping = dynamic_pointer_cast<ShipDeviceIndexToSDLDeviceIndexMapping>(mapping);
        if (sdlMapping == nullptr) {
            continue;
        }

        if (sdlMapping->GetSDLDeviceIndex() == sdlIndex) {
            return lusIndex;
        }
    }

    // didn't find one
    return PhysicalDeviceType::Max;
}

uint8_t ShipDeviceIndexMappingManager::GetPortIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId) {
    // for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
    //     auto controller = Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex);

    //     for (auto [bitmask, button] : controller->GetAllButtons()) {
    //         for (auto [id, buttonMapping] : button->GetAllButtonMappings()) {
    //             auto sdlButtonMapping = std::dynamic_pointer_cast<SDLMapping>(buttonMapping);
    //             if (sdlButtonMapping == nullptr) {
    //                 continue;
    //             }
    //             if (sdlButtonMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
    //                 return portIndex;
    //             }
    //         }
    //     }

    //     for (auto stick : { controller->GetLeftStick(), controller->GetRightStick() }) {
    //         for (auto [direction, axisDirectionMappings] : stick->GetAllAxisDirectionMappings()) {
    //             for (auto [id, axisDirectionMapping] : axisDirectionMappings) {
    //                 auto sdlAxisDirectionMapping = std::dynamic_pointer_cast<SDLMapping>(axisDirectionMapping);
    //                 if (sdlAxisDirectionMapping == nullptr) {
    //                     continue;
    //                 }
    //                 if (sdlAxisDirectionMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
    //                     return portIndex;
    //                 }
    //             }
    //         }
    //     }

    //     auto sdlGyroMapping = std::dynamic_pointer_cast<SDLMapping>(controller->GetGyro()->GetGyroMapping());
    //     if (sdlGyroMapping != nullptr && sdlGyroMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
    //         return portIndex;
    //     }

    //     for (auto [id, rumbleMapping] : controller->GetRumble()->GetAllRumbleMappings()) {
    //         auto sdlRumbleMapping = std::dynamic_pointer_cast<SDLMapping>(rumbleMapping);
    //         if (sdlRumbleMapping == nullptr) {
    //             continue;
    //         }
    //         if (sdlRumbleMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
    //             return portIndex;
    //         }
    //     }

    //     for (auto [id, ledMapping] : controller->GetLED()->GetAllLEDMappings()) {
    //         auto sdlLEDMapping = std::dynamic_pointer_cast<SDLMapping>(ledMapping);
    //         if (sdlLEDMapping == nullptr) {
    //             continue;
    //         }
    //         if (sdlLEDMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
    //             return portIndex;
    //         }
    //     }
    // }

    // couldn't find one
    return UINT8_MAX;
}

PhysicalDeviceType
ShipDeviceIndexMappingManager::GetShipDeviceIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId) {
    auto shipDeviceIndex = PhysicalDeviceType::Max;

    // for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
    //     auto controller = Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex);

    //     for (auto [bitmask, button] : controller->GetAllButtons()) {
    //         for (auto [id, buttonMapping] : button->GetAllButtonMappings()) {
    //             auto sdlButtonMapping = std::dynamic_pointer_cast<SDLMapping>(buttonMapping);
    //             if (sdlButtonMapping == nullptr) {
    //                 continue;
    //             }
    //             if (sdlButtonMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
    //                 shipDeviceIndex = sdlButtonMapping->GetPhysicalDeviceType();
    //                 sdlButtonMapping->CloseController();
    //             }
    //         }
    //     }

    //     for (auto stick : { controller->GetLeftStick(), controller->GetRightStick() }) {
    //         for (auto [direction, axisDirectionMappings] : stick->GetAllAxisDirectionMappings()) {
    //             for (auto [id, axisDirectionMapping] : axisDirectionMappings) {
    //                 auto sdlAxisDirectionMapping = std::dynamic_pointer_cast<SDLMapping>(axisDirectionMapping);
    //                 if (sdlAxisDirectionMapping == nullptr) {
    //                     continue;
    //                 }
    //                 if (sdlAxisDirectionMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
    //                     shipDeviceIndex = sdlAxisDirectionMapping->GetPhysicalDeviceType();
    //                     sdlAxisDirectionMapping->CloseController();
    //                 }
    //             }
    //         }
    //     }

    //     auto sdlGyroMapping = std::dynamic_pointer_cast<SDLMapping>(controller->GetGyro()->GetGyroMapping());
    //     if (sdlGyroMapping != nullptr && sdlGyroMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
    //         shipDeviceIndex = sdlGyroMapping->GetPhysicalDeviceType();
    //         sdlGyroMapping->CloseController();
    //     }

    //     for (auto [id, rumbleMapping] : controller->GetRumble()->GetAllRumbleMappings()) {
    //         auto sdlRumbleMapping = std::dynamic_pointer_cast<SDLMapping>(rumbleMapping);
    //         if (sdlRumbleMapping == nullptr) {
    //             continue;
    //         }
    //         if (sdlRumbleMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
    //             shipDeviceIndex = sdlRumbleMapping->GetPhysicalDeviceType();
    //             sdlRumbleMapping->CloseController();
    //         }
    //     }

    //     for (auto [id, ledMapping] : controller->GetLED()->GetAllLEDMappings()) {
    //         auto sdlLEDMapping = std::dynamic_pointer_cast<SDLMapping>(ledMapping);
    //         if (sdlLEDMapping == nullptr) {
    //             continue;
    //         }
    //         if (sdlLEDMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
    //             shipDeviceIndex = sdlLEDMapping->GetPhysicalDeviceType();
    //             sdlLEDMapping->CloseController();
    //         }
    //     }
    // }

    return shipDeviceIndex;
}

PhysicalDeviceType
ShipDeviceIndexMappingManager::GetLowestShipDeviceIndexWithNoAssociatedButtonOrAxisDirectionMappings() {
    return PhysicalDeviceType::SDLGamepad;
}

void ShipDeviceIndexMappingManager::SaveMappingIdsToConfig() {
    // todo: this efficently (when we build out cvar array support?)

    std::set<std::string> ids;
    std::stringstream mappingIdsStringStream(CVarGetString(CVAR_PREFIX_CONTROLLERS ".DeviceMappingIds", ""));
    std::string mappingIdString;
    while (getline(mappingIdsStringStream, mappingIdString, ',')) {
        ids.insert(mappingIdString);
    }

    for (auto [lusIndex, mapping] : mShipDeviceIndexToPhysicalDeviceIndexMappings) {
        ids.insert(mapping->GetMappingId());
    }

    std::string mappingIdListString = "";
    for (auto id : ids) {
        mappingIdListString += id;
        mappingIdListString += ",";
    }

    if (mappingIdListString == "") {
        CVarClear(CVAR_PREFIX_CONTROLLERS ".DeviceMappingIds");
    } else {
        CVarSetString(CVAR_PREFIX_CONTROLLERS ".DeviceMappingIds", mappingIdListString.c_str());
    }

    CVarSave();
}

std::unordered_map<PhysicalDeviceType, std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>>
ShipDeviceIndexMappingManager::GetAllDeviceIndexMappingsFromConfig() {
    std::unordered_map<PhysicalDeviceType, std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>> mappings;

    // todo: this efficently (when we build out cvar array support?)
    // i don't expect it to really be a problem with the small number of mappings we have
    // for each controller (especially compared to include/exclude locations in rando), and
    // the audio editor pattern doesn't work for this because that looks for ids that are either
    // hardcoded or provided by an otr file
    std::stringstream mappingIdsStringStream(CVarGetString(CVAR_PREFIX_CONTROLLERS ".DeviceMappingIds", ""));
    std::string mappingIdString;
    while (getline(mappingIdsStringStream, mappingIdString, ',')) {
        auto mapping = CreateDeviceIndexMappingFromConfig(mappingIdString);
        if (mapping != nullptr) {
            mappings[mapping->GetPhysicalDeviceType()] = mapping;
        }
    }

    return mappings;
}

std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>
ShipDeviceIndexMappingManager::GetDeviceIndexMappingFromShipDeviceIndex(PhysicalDeviceType lusIndex) {
    if (!mShipDeviceIndexToPhysicalDeviceIndexMappings.contains(lusIndex)) {
        return nullptr;
    }

    return mShipDeviceIndexToPhysicalDeviceIndexMappings[lusIndex];
}

std::unordered_map<PhysicalDeviceType, std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping>>
ShipDeviceIndexMappingManager::GetAllDeviceIndexMappings() {
    return mShipDeviceIndexToPhysicalDeviceIndexMappings;
}

void ShipDeviceIndexMappingManager::SetShipDeviceIndexToPhysicalDeviceIndexMapping(
    std::shared_ptr<ShipDeviceIndexToPhysicalDeviceIndexMapping> mapping) {
    mShipDeviceIndexToPhysicalDeviceIndexMappings[mapping->GetPhysicalDeviceType()] = mapping;
}

void ShipDeviceIndexMappingManager::RemoveShipDeviceIndexToPhysicalDeviceIndexMapping(PhysicalDeviceType index) {
    mShipDeviceIndexToPhysicalDeviceIndexMappings.erase(index);
}
} // namespace Ship
