#include "ShipDKDeviceIndexMappingManager.h"
#include <SDL2/SDL.h>
#include <Utils/StringHelper.h>
#include "public/bridge/consolevariablebridge.h"
#include <vector>
#include "Context.h"
#include "ControllerDisconnectedWindow.h"
#include "ControllerReorderingWindow.h"

#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/wiiu/WiiUMapping.h"
#include "port/wiiu/WiiUImpl.h"
#else
#include "controller/controldevice/controller/mapping/sdl/SDLMapping.h"
#endif

#include <sstream>

namespace ShipDK {
ShipDKDeviceIndexMappingManager::ShipDKDeviceIndexMappingManager() : mIsInitialized(false) {
#ifdef __WIIU__
    UpdateExtensionTypesFromConfig();
#else
    UpdateControllerNamesFromConfig();
#endif
}

ShipDKDeviceIndexMappingManager::~ShipDKDeviceIndexMappingManager() {
}

#ifdef __WIIU__
void ShipDKDeviceIndexMappingManager::InitializeMappingsMultiplayer(std::vector<int32_t> wiiuDeviceChannels) {
    mShipDKDeviceIndexToPhysicalDeviceIndexMappings.clear();
    uint8_t port = 0;
    for (auto channel : wiiuDeviceChannels) {
        // todo: don't just use INT32_MAX to mean gamepad
        if (channel == INT32_MAX) {
            InitializeWiiUMappingsForPort(port, true, channel);
        } else {
            InitializeWiiUMappingsForPort(port, false, channel);
        }
        port++;
    }
    mIsInitialized = true;
    ShipDK::WiiU::SetControllersInitialized();
}

void ShipDKDeviceIndexMappingManager::InitializeWiiUMappingsForPort(uint8_t n64port, bool isGamepad, int32_t wiiuChannel) {
    KPADError error;
    KPADStatus* status = !isGamepad ? ShipDK::WiiU::GetKPADStatus(static_cast<WPADChan>(wiiuChannel), &error) : nullptr;

    if (!isGamepad && status == nullptr) {
        return;
    }

    std::vector<ShipDKDeviceIndex> matchingLusIndices;
    auto mappings = GetAllDeviceIndexMappingsFromConfig();
    for (auto [lusIndex, mapping] : mappings) {
        auto wiiuMapping = std::dynamic_pointer_cast<ShipDKDeviceIndexToWiiUDeviceIndexMapping>(mapping);
        if (wiiuMapping == nullptr) {
            continue;
        }

        if (isGamepad) {
            if (wiiuMapping->IsWiiUGamepad()) {
                matchingLusIndices.push_back(lusIndex);
            }
            continue;
        }

        if (wiiuMapping->HasEquivalentExtensionType(status->extensionType)) {
            matchingLusIndices.push_back(lusIndex);
        }
    }

    // set this device to the lowest available lus index with a compatable extension type
    for (auto lusIndex : matchingLusIndices) {
        if (GetDeviceIndexMappingFromShipDKDeviceIndex(lusIndex) != nullptr) {
            // we already loaded this one
            continue;
        }

        auto mapping = mappings[lusIndex];
        auto wiiuMapping = std::dynamic_pointer_cast<ShipDKDeviceIndexToWiiUDeviceIndexMapping>(mapping);

        if (!isGamepad) {
            wiiuMapping->SetDeviceChannel(wiiuChannel);
        }
        SetShipDKDeviceIndexToPhysicalDeviceIndexMapping(wiiuMapping);

        // if we have mappings for this LUS device on this port, we're good and don't need to move any mappings
        if (Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->HasMappingsForShipDKDeviceIndex(
                lusIndex)) {
            return;
        }

        // move mappings from other port to this one
        for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
            if (portIndex == n64port) {
                continue;
            }

            if (!Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex)->HasMappingsForShipDKDeviceIndex(
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
    auto lusIndex = GetLowestShipDKDeviceIndexWithNoAssociatedButtonOrAxisDirectionMappings();
    auto deviceIndexMapping = std::make_shared<ShipDKDeviceIndexToWiiUDeviceIndexMapping>(
        lusIndex, wiiuChannel, isGamepad, !isGamepad ? status->extensionType : -1);
    mShipDKDeviceIndexToWiiUDeviceTypes[lusIndex] = { isGamepad, !isGamepad ? status->extensionType : -1 };
    deviceIndexMapping->SaveToConfig();
    SetShipDKDeviceIndexToPhysicalDeviceIndexMapping(deviceIndexMapping);
    SaveMappingIdsToConfig();
    Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->AddDefaultMappings(lusIndex);
}

bool ShipDKDeviceIndexMappingManager::IsValidWiiUExtensionType(int32_t extensionType) {
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

std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping>
ShipDKDeviceIndexMappingManager::CreateDeviceIndexMappingFromConfig(std::string id) {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(), "");

    if (mappingClass == "ShipDKDeviceIndexToWiiUDeviceIndexMapping") {
        int32_t shipDKDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.ShipDKDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        bool isGamepad = CVarGetInteger(StringHelper::Sprintf("%s.IsGamepad", mappingCvarKey.c_str()).c_str(), false);

        int32_t wiiuDeviceChannel =
            CVarGetInteger(StringHelper::Sprintf("%s.WiiUDeviceChannel", mappingCvarKey.c_str()).c_str(), -1);

        int32_t wiiuExtensionType =
            CVarGetInteger(StringHelper::Sprintf("%s.WiiUDeviceExtensionType", mappingCvarKey.c_str()).c_str(), -1);

        if (shipDKDeviceIndex < 0 || (!isGamepad && !IsValidWiiUExtensionType(wiiuExtensionType))) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<ShipDKDeviceIndexToWiiUDeviceIndexMapping>(
            static_cast<ShipDKDeviceIndex>(shipDKDeviceIndex), isGamepad, wiiuDeviceChannel, wiiuExtensionType);
    }

    return nullptr;
}

void ShipDKDeviceIndexMappingManager::InitializeMappingsSinglePlayer() {
}

void ShipDKDeviceIndexMappingManager::UpdateExtensionTypesFromConfig() {
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

        if (mappingClass == "ShipDKDeviceIndexToWiiUDeviceIndexMapping") {
            mShipDKDeviceIndexToWiiUDeviceTypes[static_cast<ShipDKDeviceIndex>(
                CVarGetInteger(StringHelper::Sprintf("%s.ShipDKDeviceIndex", mappingCvarKey.c_str()).c_str(), -1))] = {
                CVarGetInteger(StringHelper::Sprintf("%s.IsGamepad", mappingCvarKey.c_str()).c_str(), -1),
                CVarGetInteger(StringHelper::Sprintf("%s.WiiUDeviceExtensionType", mappingCvarKey.c_str()).c_str(), -1)
            };
        }
    }
}

std::pair<bool, int32_t> ShipDKDeviceIndexMappingManager::GetWiiUDeviceTypeFromShipDKDeviceIndex(ShipDKDeviceIndex index) {
    return mShipDKDeviceIndexToWiiUDeviceTypes[index];
}

void ShipDKDeviceIndexMappingManager::HandlePhysicalDevicesChanged() {
    auto controllerDisconnectedWindow = std::dynamic_pointer_cast<ControllerDisconnectedWindow>(
        Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Controller Disconnected"));
    if (controllerDisconnectedWindow != nullptr) {
        // todo: don't use UINT8_MAX-1 to mean we don't know what controller was disconnected
        controllerDisconnectedWindow->SetPortIndexOfDisconnectedController(UINT8_MAX - 1);
        controllerDisconnectedWindow->Show();
    } else {
        // todo: log error
    }
}
#else
void ShipDKDeviceIndexMappingManager::InitializeMappingsMultiplayer(std::vector<int32_t> sdlIndices) {
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
    mShipDKDeviceIndexToPhysicalDeviceIndexMappings.clear();
    uint8_t port = 0;
    for (auto sdlIndex : sdlIndices) {
        InitializeSDLMappingsForPort(port, sdlIndex);
        port++;
    }
    mIsInitialized = true;
}

void ShipDKDeviceIndexMappingManager::InitializeSDLMappingsForPort(uint8_t n64port, int32_t sdlIndex) {
    if (!SDL_IsGameController(sdlIndex)) {
        return;
    }

    char guidString[33]; // SDL_GUID_LENGTH + 1 for null terminator
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(sdlIndex), guidString, sizeof(guidString));
    std::string sdlControllerName = SDL_GameControllerNameForIndex(sdlIndex) != nullptr
                                        ? SDL_GameControllerNameForIndex(sdlIndex)
                                        : "Game Controller";

    // find all lus indices with this guid
    std::vector<ShipDKDeviceIndex> matchingGuidLusIndices;
    auto mappings = GetAllDeviceIndexMappingsFromConfig();
    for (auto [lusIndex, mapping] : mappings) {
        auto sdlMapping = std::dynamic_pointer_cast<ShipDKDeviceIndexToSDLDeviceIndexMapping>(mapping);
        if (sdlMapping == nullptr) {
            continue;
        }

        if (sdlMapping->GetJoystickGUID() == guidString) {
            matchingGuidLusIndices.push_back(lusIndex);
        }
    }

    // set this device to the lowest available lus index with this guid
    for (auto lusIndex : matchingGuidLusIndices) {
        if (GetDeviceIndexMappingFromShipDKDeviceIndex(lusIndex) != nullptr) {
            // we already loaded this one
            continue;
        }

        auto mapping = mappings[lusIndex];
        auto sdlMapping = std::dynamic_pointer_cast<ShipDKDeviceIndexToSDLDeviceIndexMapping>(mapping);

        sdlMapping->SetSDLDeviceIndex(sdlIndex);
        SetShipDKDeviceIndexToPhysicalDeviceIndexMapping(sdlMapping);

        // if we have mappings for this LUS device on this port, we're good and don't need to move any mappings
        if (Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->HasMappingsForShipDKDeviceIndex(
                lusIndex)) {
            return;
        }

        if (!Context::GetInstance()->GetControlDeck()->IsSinglePlayerMappingMode()) {
            // move mappings from other port to this one
            for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
                if (portIndex == n64port) {
                    continue;
                }

                if (!Context::GetInstance()
                         ->GetControlDeck()
                         ->GetControllerByPort(portIndex)
                         ->HasMappingsForShipDKDeviceIndex(lusIndex)) {
                    continue;
                }

                Context::GetInstance()
                    ->GetControlDeck()
                    ->GetControllerByPort(portIndex)
                    ->MoveMappingsToDifferentController(
                        Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port), lusIndex);
                return;
            }
        }

        // we have a device index mapping but no button/axis/etc. mappings, make defaults
        Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->AddDefaultMappings(lusIndex);
        return;
    }

    // if we didn't find a mapping for this guid, make defaults
    auto lusIndex = GetLowestShipDKDeviceIndexWithNoAssociatedButtonOrAxisDirectionMappings();
    auto deviceIndexMapping = std::make_shared<ShipDKDeviceIndexToSDLDeviceIndexMapping>(lusIndex, sdlIndex, guidString,
                                                                                      sdlControllerName, 25, 25);
    mShipDKDeviceIndexToSDLControllerNames[lusIndex] = sdlControllerName;
    deviceIndexMapping->SaveToConfig();
    SetShipDKDeviceIndexToPhysicalDeviceIndexMapping(deviceIndexMapping);
    SaveMappingIdsToConfig();
    Context::GetInstance()->GetControlDeck()->GetControllerByPort(n64port)->AddDefaultMappings(lusIndex);
}

std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping>
ShipDKDeviceIndexMappingManager::CreateDeviceIndexMappingFromConfig(std::string id) {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(), "");

    if (mappingClass == "ShipDKDeviceIndexToSDLDeviceIndexMapping") {
        int32_t shipDKDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.ShipDKDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

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

        if (shipDKDeviceIndex < 0 || sdlJoystickGuid == "") {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<ShipDKDeviceIndexToSDLDeviceIndexMapping>(
            static_cast<ShipDKDeviceIndex>(shipDKDeviceIndex), sdlDeviceIndex, sdlJoystickGuid, sdlControllerName,
            stickAxisThreshold, triggerAxisThreshold);
    }

    return nullptr;
}

void ShipDKDeviceIndexMappingManager::InitializeMappingsSinglePlayer() {
    for (auto mapping : Context::GetInstance()->GetControlDeck()->GetControllerByPort(0)->GetAllMappings()) {
        auto sdlMapping = std::dynamic_pointer_cast<SDLMapping>(mapping);
        if (sdlMapping == nullptr) {
            continue;
        }

        sdlMapping->CloseController();
    }

    std::vector<int32_t> connectedSdlControllerIndices;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            connectedSdlControllerIndices.push_back(i);
        }
    }

    mShipDKDeviceIndexToPhysicalDeviceIndexMappings.clear();
    for (auto sdlIndex : connectedSdlControllerIndices) {
        InitializeSDLMappingsForPort(0, sdlIndex);
    }
    mIsInitialized = true;
}

void ShipDKDeviceIndexMappingManager::UpdateControllerNamesFromConfig() {
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

        if (mappingClass == "ShipDKDeviceIndexToSDLDeviceIndexMapping") {
            mShipDKDeviceIndexToSDLControllerNames[static_cast<ShipDKDeviceIndex>(
                CVarGetInteger(StringHelper::Sprintf("%s.ShipDKDeviceIndex", mappingCvarKey.c_str()).c_str(), -1))] =
                CVarGetString(StringHelper::Sprintf("%s.SDLControllerName", mappingCvarKey.c_str()).c_str(), "");
        }
    }
}

std::string ShipDKDeviceIndexMappingManager::GetSDLControllerNameFromShipDKDeviceIndex(ShipDKDeviceIndex index) {
    return mShipDKDeviceIndexToSDLControllerNames[index];
}

int32_t ShipDKDeviceIndexMappingManager::GetNewSDLDeviceIndexFromShipDKDeviceIndex(ShipDKDeviceIndex lusIndex) {
    for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
        auto controller = Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex);

        for (auto [bitmask, button] : controller->GetAllButtons()) {
            for (auto [id, buttonMapping] : button->GetAllButtonMappings()) {
                if (buttonMapping->GetShipDKDeviceIndex() != lusIndex) {
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
                    if (axisDirectionMapping->GetShipDKDeviceIndex() != lusIndex) {
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
        if (sdlGyroMapping != nullptr && sdlGyroMapping->GetShipDKDeviceIndex() == lusIndex) {
            return sdlGyroMapping->GetCurrentSDLDeviceIndex();
        }

        for (auto [id, rumbleMapping] : controller->GetRumble()->GetAllRumbleMappings()) {
            if (rumbleMapping->GetShipDKDeviceIndex() != lusIndex) {
                continue;
            }

            auto sdlRumbleMapping = std::dynamic_pointer_cast<SDLMapping>(rumbleMapping);
            if (sdlRumbleMapping == nullptr) {
                continue;
            }

            return sdlRumbleMapping->GetCurrentSDLDeviceIndex();
        }

        for (auto [id, ledMapping] : controller->GetLED()->GetAllLEDMappings()) {
            if (ledMapping->GetShipDKDeviceIndex() != lusIndex) {
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

void ShipDKDeviceIndexMappingManager::HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex) {
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
        InitializeSDLMappingsForPort(0, sdlDeviceIndex);
        return;
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

void ShipDKDeviceIndexMappingManager::HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId) {
    if (Context::GetInstance()->GetControlDeck()->IsSinglePlayerMappingMode()) {
        HandlePhysicalDeviceDisconnectSinglePlayer(sdlJoystickInstanceId);
    } else {
        HandlePhysicalDeviceDisconnectMultiplayer(sdlJoystickInstanceId);
    }
}

void ShipDKDeviceIndexMappingManager::HandlePhysicalDeviceDisconnectSinglePlayer(int32_t sdlJoystickInstanceId) {
    auto lusIndexOfPhysicalDeviceThatHasBeenDisconnected =
        GetShipDKDeviceIndexOfDisconnectedPhysicalDevice(sdlJoystickInstanceId);

    if (lusIndexOfPhysicalDeviceThatHasBeenDisconnected == ShipDKDeviceIndex::Max) {
        // for some reason we don't know what device was disconnected
        Context::GetInstance()->GetWindow()->GetGui()->GetGameOverlay()->TextDrawNotification(
            5, true, "Unknown device disconnected");
        InitializeMappingsSinglePlayer();
        return;
    }

    for (auto [lusIndex, mapping] : mShipDKDeviceIndexToPhysicalDeviceIndexMappings) {
        auto sdlMapping = dynamic_pointer_cast<ShipDKDeviceIndexToSDLDeviceIndexMapping>(mapping);
        if (sdlMapping == nullptr) {
            continue;
        }

        if (lusIndex == lusIndexOfPhysicalDeviceThatHasBeenDisconnected) {
            sdlMapping->SetSDLDeviceIndex(-1);
            sdlMapping->SaveToConfig();
            continue;
        }

        sdlMapping->SetSDLDeviceIndex(GetNewSDLDeviceIndexFromShipDKDeviceIndex(lusIndex));
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
        auto sdlIndexMapping = std::static_pointer_cast<ShipDKDeviceIndexToSDLDeviceIndexMapping>(deviceMapping);
        if (sdlIndexMapping != nullptr) {
            Context::GetInstance()->GetWindow()->GetGui()->GetGameOverlay()->TextDrawNotification(
                5, true, "%s disconnected", sdlIndexMapping->GetSDLControllerName().c_str());
        }
    }

    RemoveShipDKDeviceIndexToPhysicalDeviceIndexMapping(lusIndexOfPhysicalDeviceThatHasBeenDisconnected);
}

void ShipDKDeviceIndexMappingManager::HandlePhysicalDeviceDisconnectMultiplayer(int32_t sdlJoystickInstanceId) {
    auto lusIndexOfPhysicalDeviceThatHasBeenDisconnected =
        GetShipDKDeviceIndexOfDisconnectedPhysicalDevice(sdlJoystickInstanceId);

    if (lusIndexOfPhysicalDeviceThatHasBeenDisconnected == ShipDKDeviceIndex::Max) {
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

    for (auto [lusIndex, mapping] : mShipDKDeviceIndexToPhysicalDeviceIndexMappings) {
        auto sdlMapping = dynamic_pointer_cast<ShipDKDeviceIndexToSDLDeviceIndexMapping>(mapping);
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

        sdlMapping->SetSDLDeviceIndex(GetNewSDLDeviceIndexFromShipDKDeviceIndex(lusIndex));
        sdlMapping->SaveToConfig();
    }

    RemoveShipDKDeviceIndexToPhysicalDeviceIndexMapping(lusIndexOfPhysicalDeviceThatHasBeenDisconnected);
}

ShipDKDeviceIndex ShipDKDeviceIndexMappingManager::GetShipDKDeviceIndexFromSDLDeviceIndex(int32_t sdlIndex) {
    for (auto [lusIndex, mapping] : mShipDKDeviceIndexToPhysicalDeviceIndexMappings) {
        auto sdlMapping = dynamic_pointer_cast<ShipDKDeviceIndexToSDLDeviceIndexMapping>(mapping);
        if (sdlMapping == nullptr) {
            continue;
        }

        if (sdlMapping->GetSDLDeviceIndex() == sdlIndex) {
            return lusIndex;
        }
    }

    // didn't find one
    return ShipDKDeviceIndex::Max;
}

uint8_t ShipDKDeviceIndexMappingManager::GetPortIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId) {
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

ShipDKDeviceIndex
ShipDKDeviceIndexMappingManager::GetShipDKDeviceIndexOfDisconnectedPhysicalDevice(int32_t sdlJoystickInstanceId) {
    for (uint8_t portIndex = 0; portIndex < 4; portIndex++) {
        auto controller = Context::GetInstance()->GetControlDeck()->GetControllerByPort(portIndex);

        for (auto [bitmask, button] : controller->GetAllButtons()) {
            for (auto [id, buttonMapping] : button->GetAllButtonMappings()) {
                auto sdlButtonMapping = std::dynamic_pointer_cast<SDLMapping>(buttonMapping);
                if (sdlButtonMapping == nullptr) {
                    continue;
                }
                if (sdlButtonMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
                    return sdlButtonMapping->GetShipDKDeviceIndex();
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
                        return sdlAxisDirectionMapping->GetShipDKDeviceIndex();
                    }
                }
            }
        }

        auto sdlGyroMapping = std::dynamic_pointer_cast<SDLMapping>(controller->GetGyro()->GetGyroMapping());
        if (sdlGyroMapping != nullptr && sdlGyroMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
            return sdlGyroMapping->GetShipDKDeviceIndex();
        }

        for (auto [id, rumbleMapping] : controller->GetRumble()->GetAllRumbleMappings()) {
            auto sdlRumbleMapping = std::dynamic_pointer_cast<SDLMapping>(rumbleMapping);
            if (sdlRumbleMapping == nullptr) {
                continue;
            }
            if (sdlRumbleMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
                return sdlRumbleMapping->GetShipDKDeviceIndex();
            }
        }

        for (auto [id, ledMapping] : controller->GetLED()->GetAllLEDMappings()) {
            auto sdlLEDMapping = std::dynamic_pointer_cast<SDLMapping>(ledMapping);
            if (sdlLEDMapping == nullptr) {
                continue;
            }
            if (sdlLEDMapping->GetJoystickInstanceId() == sdlJoystickInstanceId) {
                return sdlLEDMapping->GetShipDKDeviceIndex();
            }
        }
    }

    // couldn't find one
    return ShipDKDeviceIndex::Max;
}
#endif

ShipDKDeviceIndex ShipDKDeviceIndexMappingManager::GetLowestShipDKDeviceIndexWithNoAssociatedButtonOrAxisDirectionMappings() {
    for (uint8_t lusIndex = ShipDKDeviceIndex::Blue; lusIndex < ShipDKDeviceIndex::Max; lusIndex++) {
        if (Context::GetInstance()->GetControlDeck()->GetControllerByPort(0)->HasMappingsForShipDKDeviceIndex(
                static_cast<ShipDKDeviceIndex>(lusIndex)) ||
            Context::GetInstance()->GetControlDeck()->GetControllerByPort(1)->HasMappingsForShipDKDeviceIndex(
                static_cast<ShipDKDeviceIndex>(lusIndex)) ||
            Context::GetInstance()->GetControlDeck()->GetControllerByPort(2)->HasMappingsForShipDKDeviceIndex(
                static_cast<ShipDKDeviceIndex>(lusIndex)) ||
            Context::GetInstance()->GetControlDeck()->GetControllerByPort(3)->HasMappingsForShipDKDeviceIndex(
                static_cast<ShipDKDeviceIndex>(lusIndex))) {
            continue;
        }
        return static_cast<ShipDKDeviceIndex>(lusIndex);
    }

    // todo: invalid?
    return ShipDKDeviceIndex::Max;
}

void ShipDKDeviceIndexMappingManager::SaveMappingIdsToConfig() {
    // todo: this efficently (when we build out cvar array support?)

    std::set<std::string> ids;
    std::stringstream mappingIdsStringStream(CVarGetString("gControllers.DeviceMappingIds", ""));
    std::string mappingIdString;
    while (getline(mappingIdsStringStream, mappingIdString, ',')) {
        ids.insert(mappingIdString);
    }

    for (auto [lusIndex, mapping] : mShipDKDeviceIndexToPhysicalDeviceIndexMappings) {
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

std::unordered_map<ShipDKDeviceIndex, std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping>>
ShipDKDeviceIndexMappingManager::GetAllDeviceIndexMappingsFromConfig() {
    std::unordered_map<ShipDKDeviceIndex, std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping>> mappings;

    // todo: this efficently (when we build out cvar array support?)
    // i don't expect it to really be a problem with the small number of mappings we have
    // for each controller (especially compared to include/exclude locations in rando), and
    // the audio editor pattern doesn't work for this because that looks for ids that are either
    // hardcoded or provided by an otr file
    std::stringstream mappingIdsStringStream(CVarGetString("gControllers.DeviceMappingIds", ""));
    std::string mappingIdString;
    while (getline(mappingIdsStringStream, mappingIdString, ',')) {
        auto mapping = CreateDeviceIndexMappingFromConfig(mappingIdString);
        mappings[mapping->GetShipDKDeviceIndex()] = mapping;
    }

    return mappings;
}

std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping>
ShipDKDeviceIndexMappingManager::GetDeviceIndexMappingFromShipDKDeviceIndex(ShipDKDeviceIndex lusIndex) {
    if (!mShipDKDeviceIndexToPhysicalDeviceIndexMappings.contains(lusIndex)) {
        return nullptr;
    }

    return mShipDKDeviceIndexToPhysicalDeviceIndexMappings[lusIndex];
}

std::unordered_map<ShipDKDeviceIndex, std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping>>
ShipDKDeviceIndexMappingManager::GetAllDeviceIndexMappings() {
    return mShipDKDeviceIndexToPhysicalDeviceIndexMappings;
}

void ShipDKDeviceIndexMappingManager::SetShipDKDeviceIndexToPhysicalDeviceIndexMapping(
    std::shared_ptr<ShipDKDeviceIndexToPhysicalDeviceIndexMapping> mapping) {
    mShipDKDeviceIndexToPhysicalDeviceIndexMappings[mapping->GetShipDKDeviceIndex()] = mapping;
}

void ShipDKDeviceIndexMappingManager::RemoveShipDKDeviceIndexToPhysicalDeviceIndexMapping(ShipDKDeviceIndex index) {
    mShipDKDeviceIndexToPhysicalDeviceIndexMappings.erase(index);
}
} // namespace ShipDK
