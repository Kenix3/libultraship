#pragma once

namespace Ship {

/**
 * @brief Identifies the category of a physical input device.
 *
 * Used throughout the controller mapping system to distinguish between
 * different classes of hardware when creating or looking up input mappings.
 */
enum PhysicalDeviceType {
    Keyboard = 0,   ///< Standard keyboard.
    Mouse = 1,      ///< Mouse (buttons and motion).
    SDLGamepad = 2, ///< SDL-managed game controller.
    Max = 3         ///< Sentinel / count of device types.
};

} // namespace Ship
