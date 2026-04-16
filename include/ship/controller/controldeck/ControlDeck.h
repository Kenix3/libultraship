#pragma once

#include "ControlPort.h"
#include <vector>
#include "ship/config/Config.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "ship/controller/physicaldevice/ConnectedPhysicalDeviceManager.h"
#include "ship/controller/physicaldevice/GlobalSDLDeviceSettings.h"
#include "ship/controller/controldevice/controller/mapping/ControllerDefaultMappings.h"
#include "ship/Component.h"

namespace Ship {

/**
 * @brief Manages all controller ports and routes input/blocking requests.
 *
 * ControlDeck is the top-level controller subsystem. It owns a fixed set of
 * ControlPort instances, one per supported controller port, and exposes the
 * game-facing API for:
 * - Writing pad state from connected controllers (via the pure-virtual WriteToPad()).
 * - Blocking / unblocking game input globally (e.g. while the ImGui overlay is focused).
 * - Forwarding keyboard and mouse events to the mapping layer.
 * - Providing access to physical device management and default-mapping configuration.
 *
 * Subclass ControlDeck to implement WriteToPad() for a specific game's pad layout.
 * Obtain the instance from Context::GetControlDeck().
 */
class ControlDeck : public Component {
  public:
    /**
     * @brief Constructs the ControlDeck and sets up the port list.
     * @param additionalBitmasks        Extra button bitmasks beyond the standard set.
     * @param controllerDefaultMappings Default mappings applied when a new device is connected.
     * @param buttonNames               Human-readable names for each button bitmask, keyed by bitmask.
     */
    ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks,
                std::shared_ptr<ControllerDefaultMappings> controllerDefaultMappings,
                std::unordered_map<CONTROLLERBUTTONS_T, std::string> buttonNames);
    ~ControlDeck();

    /**
     * @brief Initialises the controller deck and loads saved mappings from Config.
     * @param controllerBits Pointer to the byte the game uses as a bitmask of connected ports.
     */
    void Init(uint8_t* controllerBits);
    /**
     * @brief Reads controller state and writes it to the game-specific pad structure(s).
     *
     * Must be implemented by a game-specific subclass to translate Controller state
     * into whatever pad format the game expects (e.g. OSContPad on Nintendo 64).
     *
     * @param pads Pointer to the game's pad buffer.
     */
    virtual void WriteToPad(void* pads) = 0;
    /**
     * @brief Returns the pointer passed to Init() that tracks which ports have controllers.
     */
    uint8_t* GetControllerBits();
    /**
     * @brief Returns the Controller connected to the given port, or nullptr.
     * @param port Zero-based port index.
     */
    std::shared_ptr<Controller> GetControllerByPort(uint8_t port);
    /**
     * @brief Blocks all game input for the caller identified by @p blockId.
     *
     * Multiple callers may independently block input; input is only unblocked when
     * all callers have called UnblockGameInput() with their respective IDs.
     *
     * @param blockId Arbitrary identifier that distinguishes this blocker from others.
     */
    void BlockGameInput(int32_t blockId);
    /**
     * @brief Removes the block associated with @p blockId.
     * @param blockId The same ID passed to BlockGameInput().
     */
    void UnblockGameInput(int32_t blockId);
    /** @brief Returns true if any blocker has blocked gamepad game input. */
    bool GamepadGameInputBlocked();
    /** @brief Returns true if any blocker has blocked keyboard game input. */
    bool KeyboardGameInputBlocked();
    /** @brief Returns true if any blocker has blocked mouse game input. */
    bool MouseGameInputBlocked();
    /**
     * @brief Forwards a keyboard event to all controllers that have keyboard bindings.
     * @param eventType Key-down or key-up.
     * @param scancode  Platform-independent scan code.
     * @return true if any controller consumed the event.
     */
    bool ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode);
    /**
     * @brief Forwards a mouse-button event to all controllers that have mouse bindings.
     * @param isPressed true for button-down, false for button-up.
     * @param button    Mouse button identifier.
     * @return true if any controller consumed the event.
     */
    bool ProcessMouseButtonEvent(bool isPressed, MouseBtn button);

    /** @brief Returns the manager that tracks currently connected SDL/HID devices. */
    std::shared_ptr<ConnectedPhysicalDeviceManager> GetConnectedPhysicalDeviceManager();
    /** @brief Returns the global SDL device settings (dead-zone, axis scale, etc.). */
    std::shared_ptr<GlobalSDLDeviceSettings> GetGlobalSDLDeviceSettings();
    /** @brief Returns the default mapping configuration applied to new controllers. */
    std::shared_ptr<ControllerDefaultMappings> GetControllerDefaultMappings();
    /**
     * @brief Returns the full bitmask→name map for all registered buttons.
     */
    const std::unordered_map<CONTROLLERBUTTONS_T, std::string>& GetAllButtonNames() const;
    /**
     * @brief Returns the human-readable name for the given button bitmask.
     * @param bitmask Single-bit button bitmask.
     * @return Human-readable name, or an empty string if the bitmask is not registered.
     */
    std::string GetButtonNameForBitmask(CONTROLLERBUTTONS_T bitmask);

  protected:
    /**
     * @brief Returns true if *all* registered blockers have blocked game input.
     *
     * Used internally by WriteToPad() implementations to decide whether to pass
     * zeroed or real pad data to the game.
     */
    bool AllGameInputBlocked();
    std::vector<std::shared_ptr<ControlPort>> mPorts = {}; ///< One entry per controller port.

  private:
    uint8_t* mControllerBits = nullptr;
    std::unordered_map<int32_t, bool> mGameInputBlockers;
    std::shared_ptr<ConnectedPhysicalDeviceManager> mConnectedPhysicalDeviceManager;
    std::shared_ptr<GlobalSDLDeviceSettings> mGlobalSDLDeviceSettings;
    std::shared_ptr<ControllerDefaultMappings> mControllerDefaultMappings;
    std::unordered_map<CONTROLLERBUTTONS_T, std::string> mButtonNames;
};
} // namespace Ship
