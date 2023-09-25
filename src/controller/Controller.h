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
#include "ButtonMapping.h"
#include "ControllerStick.h"
#include "ControllerGyro.h"

namespace LUS {

class Controller {
  public:
    Controller(uint8_t port);
    ~Controller();

    void ReloadAllMappingsFromConfig();

    bool IsConnected() const;
    void Connect();
    void Disconnect();
    void AddButtonMapping(std::shared_ptr<ButtonMapping> mapping);
    void ClearButtonMapping(std::string uuid);
    void ClearButtonMapping(std::shared_ptr<ButtonMapping> mapping);
    void ClearAllButtonMappings();
    void ResetToDefaultMappings(int32_t sdlControllerIndex);
    std::unordered_map<std::string, std::shared_ptr<ButtonMapping>> GetAllButtonMappings();
    std::shared_ptr<ButtonMapping> GetButtonMappingByUuid(std::string uuid);
    std::shared_ptr<ControllerStick> GetLeftStick();
    std::shared_ptr<ControllerStick> GetRightStick();
    std::shared_ptr<ControllerGyro> GetGyro();
    void ReadToPad(OSContPad* pad);
    bool HasConfig();
    uint8_t GetPort();

  private:
    void LoadButtonMappingFromConfig(std::string uuid);
    void SaveButtonMappingIdsToConfig();

    std::unordered_map<std::string, std::shared_ptr<ButtonMapping>> mButtonMappings;
    std::shared_ptr<ControllerStick> mLeftStick;
    std::shared_ptr<ControllerStick> mRightStick;
    std::shared_ptr<ControllerGyro> mGyro;

    bool mIsConnected;
    uint8_t mPort;

    std::deque<OSContPad> mPadBuffer;
};
} // namespace LUS
