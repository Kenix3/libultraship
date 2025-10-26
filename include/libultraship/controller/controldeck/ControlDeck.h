#pragma once

#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/controller/controldeck/ControlPort.h"
#include <vector>
#include "ship/config/Config.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "ship/controller/physicaldevice/ConnectedPhysicalDeviceManager.h"
#include "ship/controller/physicaldevice/GlobalSDLDeviceSettings.h"
#include "ship/controller/controldevice/controller/mapping/ControllerDefaultMappings.h"
#include "libultraship/libultra/controller.h"

namespace LUS {
class ControlDeck final : public Ship::ControlDeck {
  public:
    ControlDeck();
    ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks);
    ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks,
                std::shared_ptr<Ship::ControllerDefaultMappings> controllerDefaultMappings,
                std::unordered_map<CONTROLLERBUTTONS_T, std::string> buttonNames);

    OSContPad* GetPads();
    void WriteToPad(void* pad) override;

  private:
    void WriteToOSContPad(OSContPad* pad);

    OSContPad* mPads;
};
} // namespace LUS
