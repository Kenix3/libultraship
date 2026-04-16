#pragma once

#include "mapping/ControllerButtonMapping.h"
#include <memory>
#include <unordered_map>
#include <string>
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

/**
 * @brief Aggregates all ControllerButtonMapping instances for a single logical button.
 *
 * ControllerButton represents one digital button bitmask on a Controller (e.g. the A
 * button). It can hold multiple ControllerButtonMapping instances simultaneously,
 * allowing the same logical button to be triggered by a keyboard key, a gamepad face
 * button, or a mouse click all at the same time.
 *
 * Mappings are persisted in Config and can be loaded/saved via the Config methods.
 * The class also supports "listen for raw press" mode, where the next physical input
 * received is automatically captured as a new mapping.
 */
class ControllerButton {
  public:
    /**
     * @brief Constructs a ControllerButton for a specific port and bitmask.
     * @param portIndex Zero-based port index.
     * @param bitmask   Single-bit bitmask representing this button (e.g. 0x0001 for A).
     */
    ControllerButton(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask);
    ~ControllerButton();

    /**
     * @brief Returns the mapping with the given unique ID, or nullptr if not found.
     * @param id Mapping UUID.
     */
    std::shared_ptr<ControllerButtonMapping> GetButtonMappingById(std::string id);

    /**
     * @brief Returns a copy of the full id → mapping map.
     */
    std::unordered_map<std::string, std::shared_ptr<ControllerButtonMapping>> GetAllButtonMappings();

    /**
     * @brief Adds a new mapping to this button's collection.
     * @param mapping Mapping to add. Its ID must be unique within this button.
     */
    void AddButtonMapping(std::shared_ptr<ControllerButtonMapping> mapping);

    /**
     * @brief Removes the ID from Config without destroying the in-memory mapping object.
     * @param id Mapping UUID to unregister.
     */
    void ClearButtonMappingId(std::string id);

    /**
     * @brief Removes the mapping with the given ID from both the in-memory map and Config.
     * @param id Mapping UUID to remove.
     */
    void ClearButtonMapping(std::string id);

    /**
     * @brief Removes the given mapping instance from both the in-memory map and Config.
     * @param mapping Mapping to remove.
     */
    void ClearButtonMapping(std::shared_ptr<ControllerButtonMapping> mapping);

    /**
     * @brief Applies the default mappings for the given physical device type.
     * @param physicalDeviceType Type of device (keyboard, SDL gamepad, etc.).
     */
    void AddDefaultMappings(PhysicalDeviceType physicalDeviceType);

    /**
     * @brief Loads the mapping identified by @p id from Config and adds it to the collection.
     * @param id Mapping UUID stored in Config.
     */
    void LoadButtonMappingFromConfig(std::string id);

    /**
     * @brief Writes the current set of mapping IDs to Config so they can be reloaded later.
     */
    void SaveButtonMappingIdsToConfig();

    /**
     * @brief Clears all in-memory mappings and reloads them from Config.
     */
    void ReloadAllMappingsFromConfig();

    /** @brief Removes every in-memory mapping (does not touch Config). */
    void ClearAllButtonMappings();

    /**
     * @brief Removes all mappings that target the given physical device type.
     * @param physicalDeviceType Device type whose mappings should be cleared.
     */
    void ClearAllButtonMappingsForDeviceType(PhysicalDeviceType physicalDeviceType);

    /**
     * @brief Listens for the next raw physical input and adds or replaces a mapping for it.
     *
     * If @p id is non-empty, the existing mapping with that ID is replaced; otherwise
     * a new mapping is created.
     *
     * @param bitmask Button bitmask to associate with the captured input.
     * @param id      Optional UUID of an existing mapping to replace.
     * @return true if a new mapping was captured and added.
     */
    bool AddOrEditButtonMappingFromRawPress(CONTROLLERBUTTONS_T bitmask, std::string id);

    /**
     * @brief Evaluates all active mappings and sets the corresponding bits in @p padButtons.
     * @param padButtons Reference to the pad button bitfield to update.
     */
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons);

    /**
     * @brief Forwards a keyboard event to all mappings that handle keyboard input.
     * @param eventType Key-down or key-up.
     * @param scancode  Platform-independent scan code.
     * @return true if any mapping consumed the event.
     */
    bool ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode);

    /**
     * @brief Forwards a mouse-button event to all mappings that handle mouse input.
     * @param isPressed true for button-down, false for button-up.
     * @param button    Mouse button identifier.
     * @return true if any mapping consumed the event.
     */
    bool ProcessMouseButtonEvent(bool isPressed, Ship::MouseBtn button);

    /**
     * @brief Returns true if any mapping targets the given physical device type.
     * @param physicalDeviceType Device type to query.
     */
    bool HasMappingsForPhysicalDeviceType(PhysicalDeviceType physicalDeviceType);

  private:
    uint8_t mPortIndex;
    CONTROLLERBUTTONS_T mBitmask;
    std::unordered_map<std::string, std::shared_ptr<ControllerButtonMapping>> mButtonMappings;
    std::string GetConfigNameFromBitmask(CONTROLLERBUTTONS_T bitmask);

    bool mUseEventInputToCreateNewMapping;
    KbScancode mKeyboardScancodeForNewMapping;
    MouseBtn mMouseButtonForNewMapping;
};
} // namespace Ship
