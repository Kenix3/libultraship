#include "ControllerButton.h"

#include "controller/controldevice/controller/mapping/sdl/SDLButtonToButtonMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.h"

#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>

namespace LUS {
ControllerButton::ControllerButton(uint8_t portIndex, uint16_t bitmask) : mPortIndex(portIndex), mBitmask(bitmask) {
}

ControllerButton::~ControllerButton() {
}

// todo: where should this live?
std::unordered_map<uint16_t, std::string> ButtonBitmaskToConfigButtonName = {
  { BTN_A, "A" },
  { BTN_B, "B" },
  { BTN_L, "L" },
  { BTN_R, "R" },
  { BTN_Z, "Z" },
  { BTN_START, "Start" },
  { BTN_CLEFT, "CLeft" },
  { BTN_CRIGHT, "CRight" },
  { BTN_CUP, "CUp" },
  { BTN_CDOWN, "CDown" },
  { BTN_DLEFT, "DLeft" },
  { BTN_DRIGHT, "DRight" },
  { BTN_DUP, "DUp" },
  { BTN_DDOWN, "DDown" }
};

std::unordered_map<std::string, std::shared_ptr<ControllerButtonMapping>> ControllerButton::GetAllButtonMappings() {
    return mButtonMappings;
}

std::shared_ptr<ControllerButtonMapping> ControllerButton::GetButtonMappingByUuid(std::string uuid) {
    if (!mButtonMappings.contains(uuid)) {
        return nullptr;
    }

    return mButtonMappings[uuid];
}

void ControllerButton::AddButtonMapping(std::shared_ptr<ControllerButtonMapping> mapping) {
    mButtonMappings[mapping->GetButtonMappingId()] = mapping;
}

void ControllerButton::ClearButtonMapping(std::string uuid) {
    mButtonMappings[uuid]->EraseFromConfig();
    mButtonMappings.erase(uuid);
    SaveButtonMappingIdsToConfig();
}

void ControllerButton::ClearButtonMapping(std::shared_ptr<ControllerButtonMapping> mapping) {
    ClearButtonMapping(mapping->GetButtonMappingId());
}

void ControllerButton::LoadButtonMappingFromConfig(std::string id) {
    // todo: maybe this stuff makes sense in a factory?
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(), "");
    uint16_t bitmask = CVarGetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), 0);
    if (!bitmask) {
        // all button mappings need bitmasks
        CVarClear(mappingCvarKey.c_str());
        CVarSave();
        return;
    }

    if (mappingClass == "SDLButtonToButtonMapping") {
        int32_t sdlControllerIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), 0);
        int32_t sdlControllerButton =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), 0);

        if (sdlControllerIndex < 0 || sdlControllerButton == -1) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return;
        }

        AddButtonMapping(
            std::make_shared<SDLButtonToButtonMapping>(mPortIndex, bitmask, sdlControllerIndex, sdlControllerButton));
        return;
    }

    if (mappingClass == "SDLAxisDirectionToButtonMapping") {
        int32_t sdlControllerIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), 0);
        int32_t sdlControllerAxis =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), 0);
        int32_t axisDirection =
            CVarGetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), 0);

        if (sdlControllerIndex < 0 || sdlControllerAxis == -1 || (axisDirection != -1 && axisDirection != 1)) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return;
        }

        AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(mPortIndex, bitmask, sdlControllerIndex,
                                                                           sdlControllerAxis, axisDirection));
        return;
    }
}

void ControllerButton::SaveButtonMappingIdsToConfig() {
    // todo: this efficently (when we build out cvar array support?)
    std::string buttonMappingIdListString = "";
    for (auto [id, mapping] : mButtonMappings) {
        buttonMappingIdListString += id;
        buttonMappingIdListString += ",";
    }

    const std::string buttonMappingIdsCvarKey =
        StringHelper::Sprintf("gControllers.Port%d.Buttons.%sButtonMappingIds", mPortIndex + 1, ButtonBitmaskToConfigButtonName[mBitmask].c_str());
    if (buttonMappingIdListString == "") {
        CVarClear(buttonMappingIdsCvarKey.c_str());
    } else {
        CVarSetString(buttonMappingIdsCvarKey.c_str(), buttonMappingIdListString.c_str());
    }

    CVarSave();
}

void ControllerButton::ReloadAllMappingsFromConfig() {
    mButtonMappings.clear();

    // todo: this efficently (when we build out cvar array support?)
    // i don't expect it to really be a problem with the small number of mappings we have
    // for each controller (especially compared to include/exclude locations in rando), and
    // the audio editor pattern doesn't work for this because that looks for ids that are either
    // hardcoded or provided by an otr file
    const std::string buttonMappingIdsCvarKey =
        StringHelper::Sprintf("gControllers.Port%d.Buttons.%sButtonMappingIds", mPortIndex + 1, ButtonBitmaskToConfigButtonName[mBitmask].c_str());
    std::stringstream buttonMappingIdsStringStream(CVarGetString(buttonMappingIdsCvarKey.c_str(), ""));
    std::string buttonMappingIdString;
    while (getline(buttonMappingIdsStringStream, buttonMappingIdString, ',')) {
        LoadButtonMappingFromConfig(buttonMappingIdString);
    }
}

void ControllerButton::ClearAllButtonMappings() {
    for (auto [id, mapping] : mButtonMappings) {
        mapping->EraseFromConfig();
    }
    mButtonMappings.clear();
    SaveButtonMappingIdsToConfig();
}

void ControllerButton::UpdatePad(uint16_t& padButtons) {
    for (const auto& [id, mapping] : mButtonMappings) {
        mapping->UpdatePad(padButtons);
    }
}

bool ControllerButton::AddOrEditButtonMappingFromRawPress(uint16_t bitmask, std::string uuid) {
    // sdl
    std::unordered_map<int32_t, SDL_GameController*> sdlControllers;
    bool result = false;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            sdlControllers[i] = SDL_GameControllerOpen(i);
        }
    }

    for (auto [controllerIndex, controller] : sdlControllers) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) {
                if (uuid != "") {
                    ClearButtonMapping(uuid);
                }
                
                auto mapping = std::make_shared<SDLButtonToButtonMapping>(mPortIndex, bitmask, controllerIndex, button);
                AddButtonMapping(mapping);
                mapping->SaveToConfig();
                SaveButtonMappingIdsToConfig();
                result = true;
                break;
            }
        }

        if (result) {
            break;
        }

        for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
            const auto axis = static_cast<SDL_GameControllerAxis>(i);
            const auto axisValue = SDL_GameControllerGetAxis(controller, axis) / 32767.0f;
            int32_t axisDirection = 0;
            if (axisValue < -0.7f) {
                axisDirection = NEGATIVE;
            } else if (axisValue > 0.7f) {
                axisDirection = POSITIVE;
            }

            if (axisDirection == 0) {
                continue;
            }

            if (uuid != "") {
                ClearButtonMapping(uuid);
            }

            auto mapping = std::make_shared<SDLAxisDirectionToButtonMapping>(mPortIndex, bitmask, controllerIndex, axis, axisDirection);
            AddButtonMapping(mapping);
            mapping->SaveToConfig();
            SaveButtonMappingIdsToConfig();
            result = true;
            break;
        }
    }

    for (auto [i, controller] : sdlControllers) {
        SDL_GameControllerClose(controller);
    }

    return result;
}

void ControllerButton::ResetToDefaultMappings(int32_t sdlControllerIndex) {
    ClearAllButtonMappings();

    switch (mBitmask) {
        case BTN_A:
            AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(mPortIndex, BTN_A, sdlControllerIndex, SDL_CONTROLLER_BUTTON_A));
            break;
        case BTN_B:
            AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(mPortIndex, BTN_B, sdlControllerIndex, SDL_CONTROLLER_BUTTON_B));
            break;
        case BTN_L:
            AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(mPortIndex, BTN_L, sdlControllerIndex, SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
            break;
        case BTN_R:
            AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(mPortIndex, BTN_R, sdlControllerIndex, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 1));
            break;
        case BTN_Z:
            AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(mPortIndex, BTN_Z, sdlControllerIndex, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 1));
            break;
        case BTN_START:
            AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(mPortIndex, BTN_START, sdlControllerIndex, SDL_CONTROLLER_BUTTON_START));
            break;
        case BTN_CUP:
            AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(mPortIndex, BTN_CUP, sdlControllerIndex, SDL_CONTROLLER_AXIS_RIGHTY, -1));
            break;
        case BTN_CDOWN:
            AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(mPortIndex, BTN_CDOWN, sdlControllerIndex, SDL_CONTROLLER_AXIS_RIGHTY, 1));
            break;
        case BTN_CLEFT:
            AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(mPortIndex, BTN_CLEFT, sdlControllerIndex, SDL_CONTROLLER_AXIS_RIGHTX, -1));
            break;
        case BTN_CRIGHT:
            AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(mPortIndex, BTN_CRIGHT, sdlControllerIndex, SDL_CONTROLLER_AXIS_RIGHTX, 1));
            break;
        case BTN_DUP:
            AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(mPortIndex, BTN_DUP, sdlControllerIndex, SDL_CONTROLLER_BUTTON_DPAD_UP));
            break;
        case BTN_DDOWN:
            AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(mPortIndex, BTN_DDOWN, sdlControllerIndex, SDL_CONTROLLER_BUTTON_DPAD_DOWN));
            break;
        case BTN_DLEFT:
            AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(mPortIndex, BTN_DLEFT, sdlControllerIndex, SDL_CONTROLLER_BUTTON_DPAD_LEFT));
            break;
        case BTN_DRIGHT:
            AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(mPortIndex, BTN_DRIGHT, sdlControllerIndex, SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
            break;
    }

    for (auto [uuid, mapping] : mButtonMappings) {
        mapping->SaveToConfig();
    }
    SaveButtonMappingIdsToConfig();
}
} // namespace LUS
