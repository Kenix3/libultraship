#pragma once

#include "ControlPort.h"
#include <vector>
#include <config/Config.h>
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "controller/physicaldevice/ConnectedPhysicalDeviceManager.h"
#include "controller/physicaldevice/GlobalSDLDeviceSettings.h"
#include "controller/controldevice/controller/mapping/ControllerDefaultMappings.h"

namespace Ship {

class ControlDeck {
  public:
    ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks,
                std::shared_ptr<ControllerDefaultMappings> controllerDefaultMappings);
    ~ControlDeck();

    void Init(uint8_t* controllerBits);
    virtual void WriteToPad(void* pads) = 0;
    uint8_t* GetControllerBits();
    std::shared_ptr<Controller> GetControllerByPort(uint8_t port);
    void BlockGameInput(int32_t blockId);
    void UnblockGameInput(int32_t blockId);
    bool GamepadGameInputBlocked();
    bool KeyboardGameInputBlocked();
    bool MouseGameInputBlocked();
    bool ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode);
    bool ProcessMouseButtonEvent(bool isPressed, MouseBtn button);

    std::shared_ptr<ConnectedPhysicalDeviceManager> GetConnectedPhysicalDeviceManager();
    std::shared_ptr<GlobalSDLDeviceSettings> GetGlobalSDLDeviceSettings();
    std::shared_ptr<ControllerDefaultMappings> GetControllerDefaultMappings();

  protected:
    bool AllGameInputBlocked();
    std::vector<std::shared_ptr<ControlPort>> mPorts = {};

  private:
    uint8_t* mControllerBits = nullptr;
    std::unordered_map<int32_t, bool> mGameInputBlockers;
    std::shared_ptr<ConnectedPhysicalDeviceManager> mConnectedPhysicalDeviceManager;
    std::shared_ptr<GlobalSDLDeviceSettings> mGlobalSDLDeviceSettings;
    std::shared_ptr<ControllerDefaultMappings> mControllerDefaultMappings;
};
} // namespace Ship

namespace LUS {
class ControlDeck : public Ship::ControlDeck {
  public:
    ControlDeck();
    ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks);
    ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks,
                std::shared_ptr<Ship::ControllerDefaultMappings> controllerDefaultMappings);

    OSContPad* GetPads();
    void WriteToPad(void* pad) override;

  private:
    void WriteToOSContPad(OSContPad* pad);

    OSContPad* mPads;
};
} // namespace LUS
