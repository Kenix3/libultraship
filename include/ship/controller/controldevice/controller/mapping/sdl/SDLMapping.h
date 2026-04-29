#pragma once

#include <SDL2/SDL.h>

namespace Ship {

/** @brief Identifies a two-dimensional axis component. */
enum Axis { X = 0, Y = 1 };

/** @brief Identifies the sign of an axis value (negative or positive half). */
enum AxisDirection { NEGATIVE = -1, POSITIVE = 1 };

} // namespace Ship
