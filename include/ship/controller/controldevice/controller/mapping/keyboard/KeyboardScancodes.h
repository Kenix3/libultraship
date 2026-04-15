#pragma once

#ifdef __cplusplus
#include <string>

namespace Ship {
#endif

/**
 * @brief Identifies the type of a keyboard event delivered to the input mapping system.
 */
typedef enum KbEventType {
    LUS_KB_EVENT_KEY_DOWN = 0,    ///< A key was pressed.
    LUS_KB_EVENT_KEY_UP = 1,      ///< A key was released.
    LUS_KB_EVENT_ALL_KEYS_UP = 2, ///< All keys were released simultaneously (e.g. window lost focus).
    LUS_KB_EVENT_MAX              ///< Sentinel — do not use as an event type.
} KbEventType;

/**
 * @brief Platform-independent keyboard scan codes used by the input mapping system.
 *
 * Values are based on the PS/2 set-1 scan code layout so they remain stable across
 * operating systems. Use these constants when registering keyboard bindings via the
 * ControllerButtonMapping and ControllerAxisDirectionMapping APIs.
 */
typedef enum KbScancode {
    LUS_KB_UNKNOWN = 0,
    LUS_KB_ESCAPE = 1,
    LUS_KB_1 = 2,
    LUS_KB_2 = 3,
    LUS_KB_3 = 4,
    LUS_KB_4 = 5,
    LUS_KB_5 = 6,
    LUS_KB_6 = 7,
    LUS_KB_7 = 8,
    LUS_KB_8 = 9,
    LUS_KB_9 = 10,
    LUS_KB_0 = 11,
    LUS_KB_OEM_MINUS = 12,
    LUS_KB_OEM_PLUS = 13,
    LUS_KB_BACKSPACE = 14,
    LUS_KB_TAB = 15,
    LUS_KB_Q = 16,
    LUS_KB_W = 17,
    LUS_KB_E = 18,
    LUS_KB_R = 19,
    LUS_KB_T = 20,
    LUS_KB_Y = 21,
    LUS_KB_U = 22,
    LUS_KB_I = 23,
    LUS_KB_O = 24,
    LUS_KB_P = 25,
    LUS_KB_OEM_4 = 26, ///< Open bracket ([) / brace ({) on a US keyboard.
    LUS_KB_OEM_6 = 27, ///< Close bracket (]) / brace (}) on a US keyboard.
    LUS_KB_ENTER = 28,
    LUS_KB_CONTROL = 29,
    LUS_KB_A = 30,
    LUS_KB_S = 31,
    LUS_KB_D = 32,
    LUS_KB_F = 33,
    LUS_KB_G = 34,
    LUS_KB_H = 35,
    LUS_KB_J = 36,
    LUS_KB_K = 37,
    LUS_KB_L = 38,
    LUS_KB_OEM_1 = 39, ///< Semicolon (;) / colon (:) on a US keyboard.
    LUS_KB_OEM_7 = 40, ///< Single quote (') / double quote (") on a US keyboard.
    LUS_KB_OEM_3 = 41, ///< Grave (`) / tilde (~) on a US keyboard.
    LUS_KB_SHIFT = 42,
    LUS_KB_OEM_5 = 43, ///< Backslash (\) / pipe (|) on a US keyboard.
    LUS_KB_Z = 44,
    LUS_KB_X = 45,
    LUS_KB_C = 46,
    LUS_KB_V = 47,
    LUS_KB_B = 48,
    LUS_KB_N = 49,
    LUS_KB_M = 50,
    LUS_KB_OEM_COMMA = 51,
    LUS_KB_OEM_PERIOD = 52,
    LUS_KB_OEM_2 = 53, ///< Slash (/) / question mark (?) on a US keyboard.
    LUS_KB_RSHIFT = 54,
    LUS_KB_MULTIPLY = 55, ///< Asterisk (*) on the numeric keypad.
    LUS_KB_ALT = 56,
    LUS_KB_SPACE = 57,
    LUS_KB_CAPSLOCK = 58,
    LUS_KB_F1 = 59,
    LUS_KB_F2 = 60,
    LUS_KB_F3 = 61,
    LUS_KB_F4 = 62,
    LUS_KB_F5 = 63,
    LUS_KB_F6 = 64,
    LUS_KB_F7 = 65,
    LUS_KB_F8 = 66,
    LUS_KB_F9 = 67,
    LUS_KB_F10 = 68, ///< Generally inadvisable to bind — this is a Windows system key.
    LUS_KB_PAUSE = 69,
    LUS_KB_SCROLL = 70,
    LUS_KB_NUMPAD7 = 71,
    LUS_KB_NUMPAD8 = 72,
    LUS_KB_NUMPAD9 = 73,
    LUS_KB_SUBTRACT = 74, ///< Minus (-) on the numeric keypad.
    LUS_KB_NUMPAD4 = 75,
    LUS_KB_NUMPAD5 = 76,
    LUS_KB_NUMPAD6 = 77,
    LUS_KB_ADD = 78, ///< Plus (+) on the numeric keypad.
    LUS_KB_NUMPAD1 = 79,
    LUS_KB_NUMPAD2 = 80,
    LUS_KB_NUMPAD3 = 81,
    LUS_KB_NUMPAD0 = 82,
    LUS_KB_NUMPAD_DEL = 83,
    LUS_KB_PRINTSCREEN = 84,
    LUS_KB_F11 = 87,
    LUS_KB_F12 = 88,
    LUS_KB_F13 = 124,
    LUS_KB_F14 = 125,
    LUS_KB_F15 = 126,
    LUS_KB_F16 = 127,
    LUS_KB_F17 = 128,
    LUS_KB_F18 = 129,
    LUS_KB_F19 = 130,
    LUS_KB_F20 = 131,
    LUS_KB_F21 = 132,
    LUS_KB_F22 = 133,
    LUS_KB_F23 = 134,
    LUS_KB_F24 = 135,
    LUS_KB_ARROWKEY_UP = 328,
    LUS_KB_ARROWKEY_LEFT = 331,
    LUS_KB_ARROWKEY_RIGHT = 333,
    LUS_KB_ARROWKEY_DOWN = 336,
    LUS_KB_MAX ///< Sentinel — do not use as a scan code.
} KbScancode;

/**
 * @brief Identifies a mouse button delivered to the input mapping system.
 */
typedef enum MouseBtn {
    LUS_MOUSE_BTN_LEFT,     ///< Primary (left) mouse button.
    LUS_MOUSE_BTN_MIDDLE,   ///< Middle (scroll wheel) mouse button.
    LUS_MOUSE_BTN_RIGHT,    ///< Secondary (right) mouse button.
    LUS_MOUSE_BTN_BACKWARD, ///< Backward / side button (button 4).
    LUS_MOUSE_BTN_FORWARD,  ///< Forward / side button (button 5).
    LUS_MOUSE_BTN_COUNT,    ///< Number of named mouse buttons.
    LUS_MOUSE_BTN_UNKNOWN   ///< An unrecognised or unmapped mouse button.
} MouseBtn;

/**
 * @brief Identifies a scroll-wheel direction delivered to the input mapping system.
 */
typedef enum WheelDirection {
    LUS_WHEEL_NONE,    ///< No scroll (initial or reset state).
    LUS_WHEEL_LEFT,    ///< Horizontal scroll to the left.
    LUS_WHEEL_RIGHT,   ///< Horizontal scroll to the right.
    LUS_WHEEL_UP,      ///< Vertical scroll upward.
    LUS_WHEEL_DOWN,    ///< Vertical scroll downward.
    LUS_WHEEL_UNKNOWN  ///< An unrecognised scroll direction.
} WheelDirection;

#ifdef __cplusplus
/** @brief Human-readable names for each MouseBtn enumerator. */
static std::string mouseBtnNames[7] = { "MouseLeft",    "MouseMiddle",     "MouseRight",       "MouseBackward",
                                        "MouseForward", "MOUSE_BTN_COUNT", "MOUSE_BTN_UNKNOWN" };

/** @brief Human-readable names for each WheelDirection enumerator. */
static std::string wheelDirectionNames[6] = { "LUS_WHEEL_NONE", "WheelLeft", "WheelRight",
                                              "WheelUp",        "WheelDown", "LUS_WHEEL_UNKNOWN" };
} // namespace Ship
#endif
