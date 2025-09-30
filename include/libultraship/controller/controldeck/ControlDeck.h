#pragma once

#include "ControlPort.h"
#include <vector>
#include <config/Config.h>
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "controller/physicaldevice/ConnectedPhysicalDeviceManager.h"
#include "controller/physicaldevice/GlobalSDLDeviceSettings.h"
#include "controller/controldevice/controller/mapping/ControllerDefaultMappings.h"

namespace LUS {
class ControlDeck final : public Ship::ControlDeck {
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
