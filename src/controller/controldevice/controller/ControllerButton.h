#pragma once

#include "mapping/ControllerButtonMapping.h"
#include <memory>
#include <unordered_map>
#include <string>
#include "libultraship/libultra/controller.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

#define BUTTON_BITMASKS                                                                                             \
    BTN_A, BTN_B, BTN_L, BTN_R, BTN_Z, BTN_START, BTN_CLEFT, BTN_CRIGHT, BTN_CUP, BTN_CDOWN, BTN_DLEFT, BTN_DRIGHT, \
        BTN_DUP, BTN_DDOWN

class ControllerButton {
  public:
    ControllerButton(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask);
    ~ControllerButton();

    std::shared_ptr<ControllerButtonMapping> GetButtonMappingById(std::string id);
    std::unordered_map<std::string, std::shared_ptr<ControllerButtonMapping>> GetAllButtonMappings();
    void AddButtonMapping(std::shared_ptr<ControllerButtonMapping> mapping);
    void ClearButtonMappingId(std::string id);
    void ClearButtonMapping(std::string id);
    void ClearButtonMapping(std::shared_ptr<ControllerButtonMapping> mapping);
    void AddDefaultMappings(ShipDeviceIndex shipDeviceIndex);

    void LoadButtonMappingFromConfig(std::string id);
    void SaveButtonMappingIdsToConfig();
    void ReloadAllMappingsFromConfig();
    void ClearAllButtonMappings();
    void ClearAllButtonMappingsForDevice(ShipDeviceIndex shipDeviceIndex);

    bool AddOrEditButtonMappingFromRawPress(CONTROLLERBUTTONS_T bitmask, std::string id);

    void UpdatePad(CONTROLLERBUTTONS_T& padButtons);

    bool ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode);

    bool HasMappingsForShipDeviceIndex(ShipDeviceIndex lusIndex);

  private:
    uint8_t mPortIndex;
    CONTROLLERBUTTONS_T mBitmask;
    std::unordered_map<std::string, std::shared_ptr<ControllerButtonMapping>> mButtonMappings;
    std::string GetConfigNameFromBitmask(CONTROLLERBUTTONS_T bitmask);

    bool mUseKeydownEventToCreateNewMapping;
    KbScancode mKeyboardScancodeForNewMapping;
};
} // namespace Ship
