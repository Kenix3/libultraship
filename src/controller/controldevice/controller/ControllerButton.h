#pragma once

#include "mapping/ControllerButtonMapping.h"
#include <memory>
#include <unordered_map>
#include <string>
#include "libultraship/libultra/controller.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace LUS {

#define BUTTON_BITMASKS                                                                                             \
    BTN_A, BTN_B, BTN_L, BTN_R, BTN_Z, BTN_START, BTN_CLEFT, BTN_CRIGHT, BTN_CUP, BTN_CDOWN, BTN_DLEFT, BTN_DRIGHT, \
        BTN_DUP, BTN_DDOWN

class ControllerButton {
  public:
    ControllerButton(uint8_t portIndex, uint16_t bitmask);
    ~ControllerButton();

    std::shared_ptr<ControllerButtonMapping> GetButtonMappingById(std::string id);
    std::unordered_map<std::string, std::shared_ptr<ControllerButtonMapping>> GetAllButtonMappings();
    void AddButtonMapping(std::shared_ptr<ControllerButtonMapping> mapping);
    void ClearButtonMappingId(std::string id);
    void ClearButtonMapping(std::string id);
    void ClearButtonMapping(std::shared_ptr<ControllerButtonMapping> mapping);
    void AddDefaultMappings(LUSDeviceIndex lusDeviceIndex);

    void LoadButtonMappingFromConfig(std::string id);
    void SaveButtonMappingIdsToConfig();
    void ReloadAllMappingsFromConfig();
    void ClearAllButtonMappings();
    void ClearAllButtonMappingsForDevice(LUSDeviceIndex lusDeviceIndex);

    bool AddOrEditButtonMappingFromRawPress(uint16_t bitmask, std::string id);

    void UpdatePad(uint16_t& padButtons);

#ifndef __WIIU__
    bool ProcessKeyboardEvent(LUS::KbEventType eventType, LUS::KbScancode scancode);
#endif

    bool HasMappingsForLUSDeviceIndex(LUSDeviceIndex lusIndex);

  private:
    uint8_t mPortIndex;
    uint16_t mBitmask;
    std::unordered_map<std::string, std::shared_ptr<ControllerButtonMapping>> mButtonMappings;
    std::string GetConfigNameFromBitmask(uint16_t bitmask);

    bool mUseKeydownEventToCreateNewMapping;
    KbScancode mKeyboardScancodeForNewMapping;
};
} // namespace LUS
