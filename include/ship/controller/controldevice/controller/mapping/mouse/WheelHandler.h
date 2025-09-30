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
    float GetBufferedDirectionValue(WheelDirection direction);

  private:
    float CalcDirectionValue(CoordsF& coords, WheelDirection direction);
    void UpdateAxisBuffer(float* buf, float input);

    static std::shared_ptr<WheelHandler> mInstance;

    WheelDirections mDirections;
    CoordsF mCoords;
    CoordsF mBufferedCoords;
};
} // namespace Ship
