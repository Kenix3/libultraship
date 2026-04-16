#pragma once

#include "ship/controller/controldevice/ControlDevice.h"
#include "ship/controller/controldevice/controller/Controller.h"
#include <memory>

namespace Ship {
/**
 * @brief Represents a single physical controller port on the console.
 *
 * A ControlPort holds a reference to the ControlDevice (usually a Controller)
 * currently connected to it, or nullptr when the port is empty. It is owned by
 * ControlDeck and indexed from 0.
 */
class ControlPort {
  public:
    /**
     * @brief Constructs an empty port with no device attached.
     * @param portIndex Zero-based port index.
     */
    ControlPort(uint8_t portIndex);

    /**
     * @brief Constructs a port with an initial device already connected.
     * @param portIndex Zero-based port index.
     * @param device    Device to connect immediately.
     */
    ControlPort(uint8_t portIndex, std::shared_ptr<ControlDevice> device);
    ~ControlPort();

    /**
     * @brief Attaches a device to this port, replacing any previously connected device.
     * @param device Device to connect; pass nullptr to disconnect.
     */
    void Connect(std::shared_ptr<ControlDevice> device);

    /** @brief Detaches any device currently connected to this port. */
    void Disconnect();

    /**
     * @brief Returns the device connected to this port, or nullptr if the port is empty.
     */
    std::shared_ptr<ControlDevice> GetConnectedDevice();

    /**
     * @brief Returns the connected device cast to Controller, or nullptr.
     *
     * Convenience wrapper around GetConnectedDevice() that performs a dynamic cast.
     */
    std::shared_ptr<Controller> GetConnectedController();

  private:
    uint8_t mPortIndex;
    std::shared_ptr<ControlDevice> mDevice;
};
} // namespace Ship
