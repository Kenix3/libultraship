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

namespace LUS {
class Controller : public Ship::Controller {
  public:
    Controller(uint8_t portIndex);
    Controller(uint8_t portIndex, std::vector<CONTROLLERBUTTONS_T> additionalBitmasks);

    void ReadToPad(void* pad) override;

  private:
    void ReadToOSContPad(OSContPad* pad);

    std::deque<OSContPad> mPadBuffer;
};
} // namespace LUS
