#pragma once

#include "AxisDirectionMapping.h"
#include <cstdint>
#include <memory>

namespace LUS {
class ControllerStick {
  public:
    ControllerStick();
    ~ControllerStick();

    virtual void UpdatePad(int8_t& x, int8_t& y) = 0;

  private:
    std::shared_ptr<AxisDirectionMapping> mLeftMapping;
    std::shared_ptr<AxisDirectionMapping> mRightMapping;
    std::shared_ptr<AxisDirectionMapping> mUpMapping;
    std::shared_ptr<AxisDirectionMapping> mDownMapping;
};
} // namespace LUS
