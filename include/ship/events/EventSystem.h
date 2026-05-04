/**
 * @file EventSystem.h
 * @brief Backward-compatibility shim. Use Events instead.
 * @deprecated Include ship/events/Events.h and use Ship::Events directly.
 */
#pragma once
#include "ship/events/Events.h"
namespace Ship {
using EventSystem = Events;
}
