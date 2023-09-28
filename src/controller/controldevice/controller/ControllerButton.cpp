#include "ControllerButton.h"

#include "controller/controldevice/controller/mapping/factories/ButtonMappingFactory.h"

#include "controller/controldevice/controller/mapping/sdl/SDLButtonToButtonMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardKeyToButtonMapping.h"

#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>

namespace LUS {
ControllerButton::ControllerButton(uint8_t portIndex, uint16_t bitmask) : mPortIndex(portIndex), mBitmask(bitmask) {
}

ControllerButton::~ControllerButton() {
}

// todo: where should this live?
std::unordered_map<uint16_t, std::string> ButtonBitmaskToConfigButtonName = {
    { BTN_A, "A" },     { BTN_B, "B" },         { BTN_L, "L" },         { BTN_R, "R" },
    { BTN_Z, "Z" },     { BTN_START, "Start" }, { BTN_CLEFT, "CLeft" }, { BTN_CRIGHT, "CRight" },
    { BTN_CUP, "CUp" }, { BTN_CDOWN, "CDown" }, { BTN_DLEFT, "DLeft" }, { BTN_DRIGHT, "DRight" },
    { BTN_DUP, "DUp" }, { BTN_DDOWN, "DDown" }
};

std::unordered_map<std::string, std::shared_ptr<ControllerButtonMapping>> ControllerButton::GetAllButtonMappings() {
    return mButtonMappings;
}

std::shared_ptr<ControllerButtonMapping> ControllerButton::GetButtonMappingById(std::string id) {
    if (!mButtonMappings.contains(id)) {
        return nullptr;
    }

    return mButtonMappings[id];
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
    auto mapping = ButtonMappingFactory::CreateButtonMappingFromConfig(mPortIndex, id);

    if (mapping == nullptr) {
        return;
    }

    AddButtonMapping(mapping);
}

void ControllerButton::SaveButtonMappingIdsToConfig() {
    // todo: this efficently (when we build out cvar array support?)
    std::string buttonMappingIdListString = "";
    for (auto [id, mapping] : mButtonMappings) {
        buttonMappingIdListString += id;
        buttonMappingIdListString += ",";
    }

    const std::string buttonMappingIdsCvarKey =
        StringHelper::Sprintf("gControllers.Port%d.Buttons.%sButtonMappingIds", mPortIndex + 1,
                              ButtonBitmaskToConfigButtonName[mBitmask].c_str());
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
        StringHelper::Sprintf("gControllers.Port%d.Buttons.%sButtonMappingIds", mPortIndex + 1,
                              ButtonBitmaskToConfigButtonName[mBitmask].c_str());
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

bool ControllerButton::AddOrEditButtonMappingFromRawPress(uint16_t bitmask, std::string id) {
    auto mapping = ButtonMappingFactory::CreateButtonMappingFromRawPress(mPortIndex, bitmask);
    if (mapping == nullptr) {
        return false;
    }

    if (id != "") {
        ClearButtonMapping(id);
    }

    AddButtonMapping(mapping);
    mapping->SaveToConfig();
    SaveButtonMappingIdsToConfig();
    return true;
}

bool ControllerButton::ProcessKeyboardEvent(LUS::KbEventType eventType, LUS::KbScancode scancode) {
    bool result = false;
    for (auto [id, mapping] : GetAllButtonMappings()) {
        if (mapping->GetMappingType() == MAPPING_TYPE_KEYBOARD) {
            std::shared_ptr<KeyboardKeyToButtonMapping> ktobMapping =
                std::dynamic_pointer_cast<KeyboardKeyToButtonMapping>(mapping);
            if (ktobMapping != nullptr) {
                result = result || ktobMapping->ProcessKeyboardEvent(eventType, scancode);
            }
        }
    }
    return result;
}

void ControllerButton::ResetToDefaultMappings(bool keyboard, bool sdl, int32_t sdlControllerIndex) {
    ClearAllButtonMappings();

    if (sdl) {
        for (auto mapping : ButtonMappingFactory::CreateDefaultSDLButtonMappings(mPortIndex, mBitmask, sdlControllerIndex)) {
            AddButtonMapping(mapping);
        }
    }

    if (keyboard) {
        for (auto mapping :
             ButtonMappingFactory::CreateDefaultKeyboardButtonMappings(mPortIndex, mBitmask)) {
            AddButtonMapping(mapping);
        }
    }

    for (auto [uuid, mapping] : mButtonMappings) {
        mapping->SaveToConfig();
    }
    SaveButtonMappingIdsToConfig();
}
} // namespace LUS
