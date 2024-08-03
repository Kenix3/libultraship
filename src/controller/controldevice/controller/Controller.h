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
#include "ControllerRumble.h"
#include "ControllerLED.h"
#include "controller/controldevice/ControlDevice.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

class Controller : public ControlDevice {
  public:
    Controller(uint8_t portIndex);
    Controller(uint8_t portIndex, std::vector<CONTROLLERBUTTONS_T> additionalBitmasks, IntentControls* intentControls, std::vector<uint16_t> specialButtons);
    ~Controller();

    void ReloadAllMappingsFromConfig();

    bool IsConnected() const;
    void Connect();
    void Disconnect();

    void ClearAllMappings();
    void ClearAllMappingsForDevice(ShipDeviceIndex shipDeviceIndex);
    void AddDefaultMappings(ShipDeviceIndex shipDeviceIndex);
    std::unordered_map<CONTROLLERBUTTONS_T, std::shared_ptr<ControllerButton>> GetAllButtons();
    std::unordered_map<uint16_t, std::shared_ptr<ControllerButton>> GetAllSpecialButtons();
    std::shared_ptr<ControllerButton> GetButtonByBitmask(CONTROLLERBUTTONS_T bitmask);
    std::shared_ptr<ControllerButton> GetButtonBySpecialId(uint16_t bitmask);
    std::shared_ptr<ControllerButton> GetButton(CONTROLLERBUTTONS_T bitmask, uint16_t specialButton);
    std::shared_ptr<ControllerStick> GetLeftStick();
    std::shared_ptr<ControllerStick> GetRightStick();
    std::shared_ptr<ControllerGyro> GetGyro();
    std::shared_ptr<ControllerRumble> GetRumble();
    std::shared_ptr<ControllerLED> GetLED();
    void ReadToPad(OSContPad* pad);
    bool HasConfig();
    uint8_t GetPortIndex();
    std::vector<std::shared_ptr<ControllerMapping>> GetAllMappings();

    bool ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode);

    bool HasMappingsForShipDeviceIndex(ShipDeviceIndex lusIndex);
    void MoveMappingsToDifferentController(std::shared_ptr<Controller> newController, ShipDeviceIndex lusIndex);

    static IntentControlDefinitionSet intentControlDefinitions;
  private:
    void LoadButtonMappingFromConfig(std::string id);
    void SaveButtonMappingIdsToConfig();

    std::unordered_map<CONTROLLERBUTTONS_T, std::shared_ptr<ControllerButton>> mButtons;
    std::unordered_map<CONTROLLERBUTTONS_T, std::shared_ptr<ControllerButton>> mSpecialButtons;
    IntentControls* intentControls;
    std::shared_ptr<ControllerStick> mLeftStick, mRightStick;
    std::shared_ptr<ControllerGyro> mGyro;
    std::shared_ptr<ControllerRumble> mRumble;
    std::shared_ptr<ControllerLED> mLED;

    std::deque<OSContPad> mPadBuffer;
};
} // namespace Ship
