#pragma once

#include <map>
#include <memory>
#include <string>
#include <cstdint>
#include <queue>
#include <vector>
#include <map>
#include "libultraship/libultra/controller.h"
#include "ship/utils/color.h"
#include <unordered_map>
#include "ship/controller/controldevice/controller/Controller.h"

namespace LUS {
class Controller : public Ship::Controller {
  public:
    Controller(uint8_t portIndex, std::vector<CONTROLLERBUTTONS_T> bitmasks);

    void ReadToPad(void* pad) override;

  private:
    void ReadToOSContPad(OSContPad* pad);

    std::deque<OSContPad> mPadBuffer;
};
} // namespace LUS
