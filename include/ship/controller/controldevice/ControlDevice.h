#pragma once

#include <cstdint>

namespace Ship {
/**
 * @brief Abstract base class for all input devices that can be connected to a ControlPort.
 *
 * ControlDevice is the root of the controller device hierarchy. Every concrete device
 * (currently only Controller) stores the port index it is connected to, which is used
 * throughout the mapping and config systems to namespace persisted settings.
 *
 * @see Controller, ControlPort
 */
class ControlDevice {
  public:
    /**
     * @brief Constructs a ControlDevice bound to the given port.
     * @param portIndex Zero-based controller port index (0 = port 1).
     */
    ControlDevice(uint8_t portIndex);
    virtual ~ControlDevice();

  protected:
    uint8_t mPortIndex; ///< Zero-based port index this device is connected to.
};
} // namespace Ship
