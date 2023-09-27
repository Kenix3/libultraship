#pragma once

#include "mapping/ControllerButtonMapping.h"
#include <memory>
#include <unordered_map>
#include <string>
#include "libultraship/libultra/controller.h"

namespace LUS {

#define BUTTON_BITMASKS BTN_A, BTN_B, BTN_L, BTN_R, BTN_Z, BTN_START, BTN_CLEFT, BTN_CRIGHT, BTN_CUP, BTN_CDOWN, BTN_DLEFT, BTN_DRIGHT, BTN_DUP, BTN_DDOWN 

class ControllerButton {
  public:
    ControllerButton(uint8_t portIndex, uint16_t bitmask);
    ~ControllerButton();

    std::shared_ptr<ControllerButtonMapping> GetButtonMappingById(std::string id);
    std::unordered_map<std::string, std::shared_ptr<ControllerButtonMapping>> GetAllButtonMappings();
    void AddButtonMapping(std::shared_ptr<ControllerButtonMapping> mapping);
    void ClearButtonMapping(std::string uuid);
    void ClearButtonMapping(std::shared_ptr<ControllerButtonMapping> mapping);
    void ResetToDefaultMappings(int32_t sdlControllerIndex);

    void LoadButtonMappingFromConfig(std::string uuid);
    void SaveButtonMappingIdsToConfig();
    void ReloadAllMappingsFromConfig();
    void ClearAllButtonMappings();

    bool AddOrEditButtonMappingFromRawPress(uint16_t bitmask, std::string uuid);

    void UpdatePad(uint16_t& padButtons);

  private:
    uint8_t mPortIndex;
    uint16_t mBitmask;
    std::unordered_map<std::string, std::shared_ptr<ControllerButtonMapping>> mButtonMappings;
};
}
