#pragma once

#include <map>
#include <memory>
#include <string>
#include <cstdint>
#include <queue>
#include <vector>
#include <map>
#include "libultraship/libultra/controller.h"
#include "libultraship/color.h"
#include <unordered_map>
#include "ControllerButton.h"
#include "ControllerStick.h"
#include "ControllerGyro.h"
#include "controller/controldevice/ControlDevice.h"

namespace LUS {

class Controller : public ControlDevice {
  public:
    Controller(uint8_t portIndex);
    ~Controller();

    void ReloadAllMappingsFromConfig();

    bool IsConnected() const;
    void Connect();
    void Disconnect();

    void ClearAllButtonMappings();
    void ResetToDefaultMappings(int32_t sdlControllerIndex);
    std::unordered_map<uint16_t, std::shared_ptr<ControllerButton>> GetAllButtons();
    std::shared_ptr<ControllerButton> GetButton(uint16_t bitmask);
    std::shared_ptr<ControllerStick> GetLeftStick();
    std::shared_ptr<ControllerStick> GetRightStick();
    std::shared_ptr<ControllerGyro> GetGyro();
    void ReadToPad(OSContPad* pad);
    bool HasConfig();
    uint8_t GetPort();
    bool AddOrEditButtonMappingFromRawPress(uint16_t bitmask, std::string uuid);


  private:
    void LoadButtonMappingFromConfig(std::string uuid);
    void SaveButtonMappingIdsToConfig();

    std::unordered_map<uint16_t, std::shared_ptr<ControllerButton>> mButtons;
    std::shared_ptr<ControllerStick> mLeftStick, mRightStick;
    std::shared_ptr<ControllerGyro> mGyro;

    std::deque<OSContPad> mPadBuffer;
};
} // namespace LUS

// add attachments
// mempak/rumble/transfer (for now just implement rumble)