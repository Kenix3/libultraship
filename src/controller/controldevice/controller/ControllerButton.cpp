#include "ControllerButton.h"

#include "controller/controldevice/controller/mapping/factories/ButtonMappingFactory.h"

#include "controller/controldevice/controller/mapping/keyboard/KeyboardKeyToButtonMapping.h"
#include "controller/controldevice/controller/mapping/mouse/MouseButtonToButtonMapping.h"

#include "public/bridge/consolevariablebridge.h"
#include "utils/StringHelper.h"
#include <sstream>
#include <algorithm>

#include "Context.h"
#include "window/Window.h"

namespace Ship {
ControllerButton::ControllerButton(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask)
    : mPortIndex(portIndex), mBitmask(bitmask), mUseEventInputToCreateNewMapping(false),
      mKeyboardScancodeForNewMapping(LUS_KB_UNKNOWN), mMouseButtonForNewMapping(LUS_MOUSE_BTN_UNKNOWN) {
}

ControllerButton::~ControllerButton() {
}

std::string ControllerButton::GetConfigNameFromBitmask(CONTROLLERBUTTONS_T bitmask) {
    switch (bitmask) {
        case BTN_A:
            return "A";
        case BTN_B:
            return "B";
        case BTN_L:
            return "L";
        case BTN_R:
            return "R";
        case BTN_Z:
            return "Z";
        case BTN_START:
            return "Start";
        case BTN_CLEFT:
            return "CLeft";
        case BTN_CRIGHT:
            return "CRight";
        case BTN_CUP:
            return "CUp";
        case BTN_CDOWN:
            return "CDown";
        case BTN_DLEFT:
            return "DLeft";
        case BTN_DRIGHT:
            return "DRight";
        case BTN_DUP:
            return "DUp";
        case BTN_DDOWN:
            return "DDown";
        default:
            // if we don't have a name for this bitmask,
            // which happens with additionalBitmasks provided by ports,
            // return the stringified bitmask
            return std::to_string(bitmask);
    }
}

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

void ControllerButton::ClearButtonMappingId(std::string id) {
    mButtonMappings.erase(id);
    SaveButtonMappingIdsToConfig();
}

void ControllerButton::ClearButtonMapping(std::string id) {
    mButtonMappings[id]->EraseFromConfig();
    mButtonMappings.erase(id);
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
        StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.Buttons.%sButtonMappingIds", mPortIndex + 1,
                              GetConfigNameFromBitmask(mBitmask).c_str());
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
        StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.Buttons.%sButtonMappingIds", mPortIndex + 1,
                              GetConfigNameFromBitmask(mBitmask).c_str());
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

void ControllerButton::ClearAllButtonMappingsForDeviceType(PhysicalDeviceType physicalDeviceType) {
    std::vector<std::string> mappingIdsToRemove;
    for (auto [id, mapping] : mButtonMappings) {
        if (mapping->GetPhysicalDeviceType() == physicalDeviceType) {
            mapping->EraseFromConfig();
            mappingIdsToRemove.push_back(id);
        }
    }

    for (auto id : mappingIdsToRemove) {
        auto it = mButtonMappings.find(id);
        if (it != mButtonMappings.end()) {
            mButtonMappings.erase(it);
        }
    }
    SaveButtonMappingIdsToConfig();
}

void ControllerButton::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    for (const auto& [id, mapping] : mButtonMappings) {
        mapping->UpdatePad(padButtons);
    }
}

bool ControllerButton::HasMappingsForPhysicalDeviceType(PhysicalDeviceType physicalDeviceType) {
    return std::any_of(mButtonMappings.begin(), mButtonMappings.end(), [physicalDeviceType](const auto& mapping) {
        return mapping.second->GetPhysicalDeviceType() == physicalDeviceType;
    });
}

bool ControllerButton::AddOrEditButtonMappingFromRawPress(CONTROLLERBUTTONS_T bitmask, std::string id) {
    std::shared_ptr<ControllerButtonMapping> mapping = nullptr;

    mUseEventInputToCreateNewMapping = true;
    if (mKeyboardScancodeForNewMapping != LUS_KB_UNKNOWN) {
        mapping = std::make_shared<KeyboardKeyToButtonMapping>(mPortIndex, bitmask, mKeyboardScancodeForNewMapping);
    }

    else if (!Context::GetInstance()->GetWindow()->GetGui()->IsMouseOverAnyGuiItem() &&
             Context::GetInstance()->GetWindow()->GetGui()->IsMouseOverActivePopup()) {
        if (mMouseButtonForNewMapping != LUS_MOUSE_BTN_UNKNOWN) {
            mapping = std::make_shared<MouseButtonToButtonMapping>(mPortIndex, bitmask, mMouseButtonForNewMapping);
        } else {
            mapping = ButtonMappingFactory::CreateButtonMappingFromMouseWheelInput(mPortIndex, bitmask);
        }
    }

    if (mapping == nullptr) {
        mapping = ButtonMappingFactory::CreateButtonMappingFromSDLInput(mPortIndex, bitmask);
    }

    if (mapping == nullptr) {
        return false;
    }

    mKeyboardScancodeForNewMapping = LUS_KB_UNKNOWN;
    mMouseButtonForNewMapping = LUS_MOUSE_BTN_UNKNOWN;
    mUseEventInputToCreateNewMapping = false;

    if (id != "") {
        ClearButtonMapping(id);
    }

    AddButtonMapping(mapping);
    mapping->SaveToConfig();
    SaveButtonMappingIdsToConfig();
    const std::string hasConfigCvarKey =
        StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.HasConfig", mPortIndex + 1);
    CVarSetInteger(hasConfigCvarKey.c_str(), true);
    CVarSave();
    return true;
}

bool ControllerButton::ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode) {
    if (mUseEventInputToCreateNewMapping) {
        if (eventType == LUS_KB_EVENT_KEY_DOWN) {
            mKeyboardScancodeForNewMapping = scancode;
            return true;
        } else {
            mKeyboardScancodeForNewMapping = LUS_KB_UNKNOWN;
        }
    }

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

bool ControllerButton::ProcessMouseButtonEvent(bool isPressed, MouseBtn button) {
    if (mUseEventInputToCreateNewMapping) {
        if (isPressed) {
            mMouseButtonForNewMapping = button;
            return true;
        } else {
            mMouseButtonForNewMapping = LUS_MOUSE_BTN_UNKNOWN;
        }
    }

    bool result = false;
    for (auto [id, mapping] : GetAllButtonMappings()) {
        if (mapping->GetMappingType() == MAPPING_TYPE_MOUSE) {
            std::shared_ptr<MouseButtonToButtonMapping> mtobMapping =
                std::dynamic_pointer_cast<MouseButtonToButtonMapping>(mapping);
            if (mtobMapping != nullptr) {
                result = result || mtobMapping->ProcessMouseButtonEvent(isPressed, button);
            }
        }
    }
    return result;
}

void ControllerButton::AddDefaultMappings(PhysicalDeviceType physicalDeviceType) {
    if (physicalDeviceType == PhysicalDeviceType::SDLGamepad) {
        for (auto mapping : ButtonMappingFactory::CreateDefaultSDLButtonMappings(mPortIndex, mBitmask)) {
            AddButtonMapping(mapping);
        }
    }

    if (physicalDeviceType == PhysicalDeviceType::Keyboard) {
        for (auto mapping : ButtonMappingFactory::CreateDefaultKeyboardButtonMappings(mPortIndex, mBitmask)) {
            AddButtonMapping(mapping);
        }
    }

    for (auto [id, mapping] : mButtonMappings) {
        mapping->SaveToConfig();
    }
    SaveButtonMappingIdsToConfig();
}
} // namespace Ship
