#pragma once

namespace Ship {
class IControllerAxisRemapper {
  public:
    virtual ~IControllerAxisRemapper() = default;
    virtual void RemapStick(double& x, double& y) = 0;
};
}