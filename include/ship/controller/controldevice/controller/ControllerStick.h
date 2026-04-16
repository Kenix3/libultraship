#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

/** @brief Default stick sensitivity as a percentage of full scale. */
#define DEFAULT_STICK_SENSITIVITY_PERCENTAGE 100
/** @brief Default stick dead-zone as a percentage of full scale. */
#define DEFAULT_STICK_DEADZONE_PERCENTAGE 20
/** @brief Default notch snap angle in degrees (0 = disabled). */
#define DEFAULT_NOTCH_SNAP_ANGLE 0

/**
 * @brief Aggregates axis-direction mappings and applies sensitivity, dead-zone, and notch-snap to a stick.
 *
 * ControllerStick tracks four directional axes (Up, Down, Left, Right) and can hold
 * multiple ControllerAxisDirectionMapping instances per direction. Every frame, UpdatePad()
 * reads raw axis values from the mappings, applies the configured dead-zone and
 * sensitivity, optionally snaps to 8-way notch angles, and writes the result as
 * signed 8-bit values.
 *
 * Settings (sensitivity, dead-zone, notch snap angle) are persisted in Config.
 */
class ControllerStick {
  public:
    /**
     * @brief Constructs a ControllerStick for a given port and stick index.
     * @param portIndex  Zero-based port index.
     * @param stickIndex Stick identifier (e.g. LEFT_STICK or RIGHT_STICK).
     */
    ControllerStick(uint8_t portIndex, StickIndex stickIndex);
    ~ControllerStick();

    /** @brief Clears all in-memory mappings and reloads them from Config. */
    void ReloadAllMappingsFromConfig();

    /**
     * @brief Applies the default axis-direction mappings for the given device type.
     * @param physicalDeviceType Device type to apply defaults for.
     */
    void AddDefaultMappings(PhysicalDeviceType physicalDeviceType);

    /** @brief Removes all axis-direction mappings for all directions. */
    void ClearAllMappings();

    /**
     * @brief Removes all axis-direction mappings that target the given device type.
     * @param physicalDeviceType Device type whose mappings should be cleared.
     */
    void ClearAllMappingsForDeviceType(PhysicalDeviceType physicalDeviceType);

    /**
     * @brief Evaluates all axis mappings and writes the processed stick values to @p x and @p y.
     *
     * Dead-zone, sensitivity, and notch-snap are applied before writing the output.
     *
     * @param x Output X axis value in the range [-128, 127].
     * @param y Output Y axis value in the range [-128, 127].
     */
    void UpdatePad(int8_t& x, int8_t& y);

    /**
     * @brief Returns the mapping for the given direction and ID, or nullptr if not found.
     * @param direction Axis direction (Up/Down/Left/Right).
     * @param id        Mapping UUID.
     */
    std::shared_ptr<ControllerAxisDirectionMapping> GetAxisDirectionMappingById(Direction direction, std::string id);

    /**
     * @brief Returns a copy of the full direction → (id → mapping) map.
     */
    std::unordered_map<Direction, std::unordered_map<std::string, std::shared_ptr<ControllerAxisDirectionMapping>>>
    GetAllAxisDirectionMappings();

    /**
     * @brief Returns the id → mapping map for a single direction.
     * @param direction Axis direction to query.
     */
    std::unordered_map<std::string, std::shared_ptr<ControllerAxisDirectionMapping>>
    GetAllAxisDirectionMappingByDirection(Direction direction);

    /**
     * @brief Adds a mapping for the given direction.
     * @param direction Axis direction.
     * @param mapping   Mapping to add.
     */
    void AddAxisDirectionMapping(Direction direction, std::shared_ptr<ControllerAxisDirectionMapping> mapping);

    /**
     * @brief Removes the mapping ID from Config without destroying the in-memory object.
     * @param direction Axis direction.
     * @param id        Mapping UUID.
     */
    void ClearAxisDirectionMappingId(Direction direction, std::string id);

    /**
     * @brief Removes the mapping by ID from both in-memory and Config.
     * @param direction Axis direction.
     * @param id        Mapping UUID.
     */
    void ClearAxisDirectionMapping(Direction direction, std::string id);

    /**
     * @brief Removes the given mapping instance from both in-memory and Config.
     * @param direction Axis direction.
     * @param mapping   Mapping to remove.
     */
    void ClearAxisDirectionMapping(Direction direction, std::shared_ptr<ControllerAxisDirectionMapping> mapping);

    /** @brief Writes the current set of mapping IDs for all directions to Config. */
    void SaveAxisDirectionMappingIdsToConfig();

    /**
     * @brief Listens for the next raw physical input and adds or replaces a directional mapping.
     * @param direction Axis direction the captured input should drive.
     * @param id        Optional UUID of an existing mapping to replace; empty = create new.
     * @return true if a mapping was captured and added.
     */
    bool AddOrEditAxisDirectionMappingFromRawPress(Direction direction, std::string id);

    /**
     * @brief Reads raw axis values, applies processing, and writes to @p x and @p y.
     *
     * Alias of UpdatePad() used during the game's input-read phase.
     */
    void Process(int8_t& x, int8_t& y);

    /** @brief Resets sensitivity to DEFAULT_STICK_SENSITIVITY_PERCENTAGE and saves to Config. */
    void ResetSensitivityToDefault();

    /**
     * @brief Sets the stick sensitivity and saves to Config.
     * @param sensitivityPercentage Sensitivity as a percentage (100 = full scale).
     */
    void SetSensitivity(uint8_t sensitivityPercentage);

    /** @brief Returns the current sensitivity percentage. */
    uint8_t GetSensitivityPercentage();

    /** @brief Returns true if sensitivity is at the default value. */
    bool SensitivityIsDefault();

    /** @brief Resets dead-zone to DEFAULT_STICK_DEADZONE_PERCENTAGE and saves to Config. */
    void ResetDeadzoneToDefault();

    /**
     * @brief Sets the dead-zone and saves to Config.
     * @param deadzonePercentage Dead-zone radius as a percentage of full scale.
     */
    void SetDeadzone(uint8_t deadzonePercentage);

    /** @brief Returns the current dead-zone percentage. */
    uint8_t GetDeadzonePercentage();

    /** @brief Returns true if dead-zone is at the default value. */
    bool DeadzoneIsDefault();

    /** @brief Resets notch snap angle to DEFAULT_NOTCH_SNAP_ANGLE and saves to Config. */
    void ResetNotchSnapAngleToDefault();

    /**
     * @brief Sets the notch snap angle and saves to Config.
     * @param notchSnapAngle Snap threshold in degrees (0 = disabled).
     */
    void SetNotchSnapAngle(uint8_t notchSnapAngle);

    /** @brief Returns the current notch snap angle in degrees. */
    uint8_t GetNotchSnapAngle();

    /** @brief Returns true if the notch snap angle is at the default value. */
    bool NotchSnapAngleIsDefault();

    /**
     * @brief Forwards a keyboard event to all directional mappings that handle keyboard input.
     * @param eventType Key-down or key-up.
     * @param scancode  Platform-independent scan code.
     * @return true if any mapping consumed the event.
     */
    bool ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode);

    /**
     * @brief Forwards a mouse-button event to all directional mappings that handle mouse input.
     * @param isPressed true for button-down, false for button-up.
     * @param button    Mouse button identifier.
     * @return true if any mapping consumed the event.
     */
    bool ProcessMouseButtonEvent(bool isPressed, Ship::MouseBtn button);

    /**
     * @brief Returns true if any directional mapping targets the given device type.
     * @param physicalDeviceType Device type to query.
     */
    bool HasMappingsForPhysicalDeviceType(PhysicalDeviceType physicalDeviceType);

    /** @brief Returns the stick index (left or right). */
    StickIndex GetStickIndex();

  private:
    double GetClosestNotch(double angle, double approximationThreshold);
    void LoadAxisDirectionMappingFromConfig(std::string id);
    float GetAxisDirectionValue(Direction direction);

    uint8_t mPortIndex;
    StickIndex mStickIndex;

    uint8_t mSensitivityPercentage;
    float mSensitivity;

    // TODO: handle deadzones separately for X and Y?
    uint8_t mDeadzonePercentage;
    float mDeadzone;
    uint8_t mNotchSnapAngle;

    std::unordered_map<Direction, std::unordered_map<std::string, std::shared_ptr<ControllerAxisDirectionMapping>>>
        mAxisDirectionMappings;

    bool mUseEventInputToCreateNewMapping;
    KbScancode mKeyboardScancodeForNewMapping;
    MouseBtn mMouseButtonForNewMapping;
};
} // namespace Ship
