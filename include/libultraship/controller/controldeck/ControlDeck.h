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
/**
 * @brief Ship's N64-compatible ControlDeck implementation.
 *
 * LUS::ControlDeck is the concrete ControlDeck subclass used by libultraship. It
 * owns an OSContPad buffer (one entry per port) and implements WriteToPad() to copy
 * the aggregated controller state from each Ship::Controller into that buffer for
 * consumption by the emulated game.
 *
 * The default constructor registers the standard N64 button set. The two-argument
 * and three-argument constructors allow a game to extend the button set with custom
 * bitmasks and to provide its own default mapping tables.
 *
 * Obtain the instance from Context::GetControlDeck(), cast to LUS::ControlDeck if
 * you need access to GetPads().
 */
class ControlDeck final : public Ship::ControlDeck {
  public:
    /**
     * @brief Constructs a ControlDeck with the standard N64 button set and default mappings.
     */
    ControlDeck();

    /**
     * @brief Constructs a ControlDeck with the standard N64 buttons plus extra bitmasks.
     * @param additionalBitmasks Extra button bitmasks beyond the standard N64 set.
     */
    ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks);

    /**
     * @brief Full constructor: extra bitmasks, custom default mappings, and custom button names.
     * @param additionalBitmasks        Extra button bitmasks.
     * @param controllerDefaultMappings Default mappings applied to new devices.
     * @param buttonNames               Human-readable names for each button bitmask.
     */
    ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks,
                std::shared_ptr<Ship::ControllerDefaultMappings> controllerDefaultMappings,
                std::unordered_map<CONTROLLERBUTTONS_T, std::string> buttonNames);

    /**
     * @brief Returns a pointer to the OSContPad buffer (one pad per port).
     *
     * The buffer should be updated by WriteToPad() at the start of each game frame.
     * The lifetime of the returned pointer is tied to this ControlDeck.
     */
    OSContPad* GetPads();

    /**
     * @brief Reads each connected controller and writes the result to the game's pad buffer.
     *
     * @p pad must point to an array of OSContPad structures large enough to hold one
     * entry per port (i.e. at least MAXCONTROLLERS * sizeof(OSContPad) bytes).
     *
     * @param pad Pointer to the game's OSContPad buffer.
     */
    void WriteToPad(void* pad) override;

  private:
    /**
     * @brief Translates a single port's Ship::Controller state to one OSContPad entry.
     * @param pad Pointer to the OSContPad entry for this port.
     */
    void WriteToOSContPad(OSContPad* pad);

    OSContPad* mPads; ///< OSContPad array (one entry per port), owned by this class.
};
} // namespace LUS
