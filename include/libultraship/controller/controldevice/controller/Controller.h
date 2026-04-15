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
/**
 * @brief Ship's N64-compatible Controller implementation.
 *
 * LUS::Controller is the concrete Controller subclass used by libultraship. It
 * stores a rolling buffer of OSContPad states and implements ReadToPad() to convert
 * the aggregated Ship::Controller input state into a single OSContPad entry.
 *
 * The pad buffer allows the game to read back previous frames of input if needed
 * (e.g. for input prediction or replay).
 *
 * Instances are created automatically by LUS::ControlDeck when a port is configured;
 * access them via ControlDeck::GetControllerByPort().
 */
class Controller : public Ship::Controller {
  public:
    /**
     * @brief Constructs a Controller for the given port with the specified button bitmasks.
     * @param portIndex Zero-based port index (0 = port 1).
     * @param bitmasks  All button bitmasks this controller should track.
     */
    Controller(uint8_t portIndex, std::vector<CONTROLLERBUTTONS_T> bitmasks);

    /**
     * @brief Reads the current Ship::Controller state and writes it to the game's pad structure.
     *
     * @p pad must point to the OSContPad entry for this port within the deck's pad buffer.
     *
     * @param pad Pointer to the OSContPad entry to fill.
     */
    void ReadToPad(void* pad) override;

  private:
    /**
     * @brief Converts the current Ship input state into one OSContPad entry.
     * @param pad Pointer to the OSContPad entry to fill.
     */
    void ReadToOSContPad(OSContPad* pad);

    std::deque<OSContPad> mPadBuffer; ///< Rolling history of pad states (newest at front).
};
} // namespace LUS
