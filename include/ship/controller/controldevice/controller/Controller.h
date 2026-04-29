#pragma once

#include <map>
#include <memory>
#include <string>
#include <cstdint>
#include <queue>
#include <vector>
#include <map>
#include <unordered_map>
#include "ControllerButton.h"
#include "ControllerStick.h"
#include "ControllerGyro.h"
#include "ControllerRumble.h"
#include "ControllerLED.h"
#include "ship/controller/controldevice/ControlDevice.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

/**
 * @brief Represents a single logical controller connected to a ControlPort.
 *
 * Controller aggregates a set of ControllerButton, two ControllerStick, a
 * ControllerGyro, a ControllerRumble, and a ControllerLED — each of which holds
 * a collection of ControllerMapping instances that translate physical device events
 * into logical input.
 *
 * Subclass Controller and implement ReadToPad() to convert the aggregated input state
 * into whatever structure the game expects (e.g. an N64 OSContPad).
 *
 * Mappings are persisted in Config under a key derived from the port index and are
 * (re-)loaded via ReloadAllMappingsFromConfig().
 */
class Controller : public ControlDevice {
  public:
    /**
     * @brief Constructs a Controller for the given port with a set of button bitmasks.
     * @param portIndex Zero-based port index.
     * @param bitmasks  All button bitmasks this controller should track (one ControllerButton per entry).
     */
    Controller(uint8_t portIndex, std::vector<CONTROLLERBUTTONS_T> bitmasks);

    /** @brief Destroys the Controller and releases all mapping resources. */
    ~Controller();

    /**
     * @brief Discards all in-memory mappings and reloads them from Config.
     *
     * Called on startup and whenever the user saves mapping changes.
     */
    void ReloadAllMappingsFromConfig();

    /** @brief Returns true if at least one physical device mapping is active for any button. */
    bool IsConnected() const;

    /** @brief Marks the controller as "connected" (used by the mapping layer). */
    void Connect();

    /** @brief Marks the controller as "disconnected" and clears transient state. */
    void Disconnect();

    /** @brief Removes all mappings for all buttons, sticks, rumble, LED, and gyro. */
    void ClearAllMappings();

    /**
     * @brief Removes all mappings that target the given physical device type.
     * @param physicalDeviceType Type of device whose mappings should be cleared (e.g. keyboard, SDL gamepad).
     */
    void ClearAllMappingsForDeviceType(PhysicalDeviceType physicalDeviceType);

    /**
     * @brief Applies the default mappings for the given physical device type.
     * @param physicalDeviceType Device type to apply defaults for.
     */
    void AddDefaultMappings(PhysicalDeviceType physicalDeviceType);

    /**
     * @brief Returns the full bitmask→ControllerButton map.
     */
    std::unordered_map<CONTROLLERBUTTONS_T, std::shared_ptr<ControllerButton>> GetAllButtons();

    /**
     * @brief Returns the ControllerButton for the given bitmask, or nullptr if not found.
     * @param bitmask Single-bit button bitmask.
     */
    std::shared_ptr<ControllerButton> GetButtonByBitmask(CONTROLLERBUTTONS_T bitmask);

    /**
     * @brief Alias of GetButtonByBitmask().
     * @param bitmask Single-bit button bitmask.
     */
    std::shared_ptr<ControllerButton> GetButton(CONTROLLERBUTTONS_T bitmask);

    /** @brief Returns the left analog stick input aggregator. */
    std::shared_ptr<ControllerStick> GetLeftStick();

    /** @brief Returns the right analog stick input aggregator. */
    std::shared_ptr<ControllerStick> GetRightStick();

    /** @brief Returns the gyroscope input aggregator. */
    std::shared_ptr<ControllerGyro> GetGyro();

    /** @brief Returns the rumble output aggregator. */
    std::shared_ptr<ControllerRumble> GetRumble();

    /** @brief Returns the LED output aggregator. */
    std::shared_ptr<ControllerLED> GetLED();

    /**
     * @brief Reads aggregated input state and writes it to the game-specific pad structure.
     *
     * Must be implemented by a game-specific subclass.
     *
     * @param pad Pointer to the game's pad buffer for this controller's port.
     */
    virtual void ReadToPad(void* pad) = 0;

    /**
     * @brief Returns true if any Config key exists for this controller's mappings.
     */
    bool HasConfig();

    /** @brief Returns the zero-based port index this controller is bound to. */
    uint8_t GetPortIndex();

    /**
     * @brief Returns a flat list of all ControllerMapping instances across all subsystems.
     */
    std::vector<std::shared_ptr<ControllerMapping>> GetAllMappings();

    /**
     * @brief Forwards a keyboard event to all button and stick mappings.
     * @param eventType Key-down or key-up.
     * @param scancode  Platform-independent scan code.
     * @return true if any mapping consumed the event.
     */
    bool ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode);

    /**
     * @brief Forwards a mouse-button event to all button and stick mappings.
     * @param isPressed true for button-down, false for button-up.
     * @param button    Mouse button identifier.
     * @return true if any mapping consumed the event.
     */
    bool ProcessMouseButtonEvent(bool isPressed, MouseBtn button);

    /**
     * @brief Returns true if any mapping for this controller targets the given physical device type.
     * @param physicalDeviceType Device type to query.
     */
    bool HasMappingsForPhysicalDeviceType(PhysicalDeviceType physicalDeviceType);

  protected:
    std::unordered_map<CONTROLLERBUTTONS_T, std::shared_ptr<ControllerButton>>
        mButtons; ///< Button subsystems keyed by bitmask.

  private:
    void LoadButtonMappingFromConfig(std::string id);
    void SaveButtonMappingIdsToConfig();

    std::shared_ptr<ControllerStick> mLeftStick, mRightStick;
    std::shared_ptr<ControllerGyro> mGyro;
    std::shared_ptr<ControllerRumble> mRumble;
    std::shared_ptr<ControllerLED> mLED;
};
} // namespace Ship
