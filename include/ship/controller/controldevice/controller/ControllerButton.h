#pragma once

#include "mapping/ControllerButtonMapping.h"
#include <memory>
#include <unordered_map>
#include <string>
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

class ControllerButton {
  public:
    ControllerButton(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                     std::unordered_map<CONTROLLERBUTTONS_T, std::string> buttonNames);
    ~ControllerButton();

    std::shared_ptr<ControllerButtonMapping> GetButtonMappingById(std::string id);
    std::unordered_map<std::string, std::shared_ptr<ControllerButtonMapping>> GetAllButtonMappings();
    void AddButtonMapping(std::shared_ptr<ControllerButtonMapping> mapping);
    void ClearButtonMappingId(std::string id);
    void ClearButtonMapping(std::string id);
    void ClearButtonMapping(std::shared_ptr<ControllerButtonMapping> mapping);
    void AddDefaultMappings(PhysicalDeviceType physicalDeviceType);

    void LoadButtonMappingFromConfig(std::string id);
    void SaveButtonMappingIdsToConfig();
    void ReloadAllMappingsFromConfig();
    void ClearAllButtonMappings();
    void ClearAllButtonMappingsForDeviceType(PhysicalDeviceType physicalDeviceType);

    bool AddOrEditButtonMappingFromRawPress(CONTROLLERBUTTONS_T bitmask, std::string id);

    void UpdatePad(CONTROLLERBUTTONS_T& padButtons);

    bool ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode);
    bool ProcessMouseButtonEvent(bool isPressed, Ship::MouseBtn button);

    bool HasMappingsForPhysicalDeviceType(PhysicalDeviceType physicalDeviceType);

  private:
    uint8_t mPortIndex;
    CONTROLLERBUTTONS_T mBitmask;
    std::unordered_map<std::string, std::shared_ptr<ControllerButtonMapping>> mButtonMappings;

    std::unordered_map<CONTROLLERBUTTONS_T, std::string> mButtonNames;
    std::string GetConfigNameFromBitmask(CONTROLLERBUTTONS_T bitmask);

    bool mUseEventInputToCreateNewMapping;
    KbScancode mKeyboardScancodeForNewMapping;
    MouseBtn mMouseButtonForNewMapping;
};
} // namespace Ship
