#pragma once

#include <memory>

#include "window/Window.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {
struct WheelDirections {
  WheelDirection x;
  WheelDirection y;
};

class WheelHandler {
  public:
    WheelHandler();
    ~WheelHandler();
    static std::shared_ptr<WheelHandler> GetInstance();

    void Update();
    CoordsF GetCoords();
    WheelDirections GetDirections();
    float GetDirectionValue(WheelDirection direction);

  private:
    static std::shared_ptr<WheelHandler> mInstance;

    WheelDirections mDirections;
    CoordsF mCoords;
};
} // namespace Ship
