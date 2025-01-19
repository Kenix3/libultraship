#pragma once

#include <cstdint>
#include <SDL2/SDL.h>
#include <memory>
#include <string>
#include "controller/controldevice/controller/mapping/ControllerMapping.h"

namespace Ship {

enum Axis { X = 0, Y = 1 };
enum AxisDirection { NEGATIVE = -1, POSITIVE = 1 };

} // namespace Ship
