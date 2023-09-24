#include "ControllerStick.h"
#include <spdlog/spdlog.h>

#include "controller/sdl/SDLAxisDirectionToAxisDirectionMapping.h"

#define M_TAU 6.2831853071795864769252867665590057 // 2 * pi
#define MINIMUM_RADIUS_TO_MAP_NOTCH 0.9

namespace LUS {
ControllerStick::ControllerStick() {
  mDeadzone = 16.0f;
  mNotchProxmityThreshold = 0;
}

ControllerStick::~ControllerStick() {
}

void ControllerStick::ClearAllMappings() {
  mLeftMapping = nullptr;
  mRightMapping = nullptr;
  mUpMapping = nullptr;
  mDownMapping = nullptr;
}

void ControllerStick::ReloadAllMappings() {
    ClearAllMappings();

    UpdateAxisDirectionMapping(LEFT, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(0, 0, -1));
    UpdateAxisDirectionMapping(RIGHT, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(0, 0, 1));
    UpdateAxisDirectionMapping(UP, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(0, 1, -1));
    UpdateAxisDirectionMapping(DOWN, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(0, 1, 1));
}

double ControllerStick::GetClosestNotch(double angle, double approximationThreshold) {
    constexpr auto octagonAngle = M_TAU / 8;
    const auto closestNotch = std::round(angle / octagonAngle) * octagonAngle;
    const auto distanceToNotch = std::abs(fmod(closestNotch - angle + M_PI, M_TAU) - M_PI);
    return distanceToNotch < approximationThreshold / 2 ? closestNotch : angle;
}

void ControllerStick::Process(int8_t& x, int8_t& y) {
    auto sx = mRightMapping->GetNormalizedAxisDirectionValue() - mLeftMapping->GetNormalizedAxisDirectionValue();
    auto sy = mUpMapping->GetNormalizedAxisDirectionValue() - mDownMapping->GetNormalizedAxisDirectionValue();

    auto ux = fabs(sx);
    auto uy = fabs(sy);

    // create scaled circular dead-zone
    auto len = sqrt(ux * ux + uy * uy);
    if (len < mDeadzone) {
        len = 0;
    } else if (len > MAX_AXIS_RANGE) {
        len = MAX_AXIS_RANGE / len;
    } else {
        len = (len - mDeadzone) * MAX_AXIS_RANGE / (MAX_AXIS_RANGE - mDeadzone) / len;
    }
    ux *= len;
    uy *= len;

    // bound diagonals to an octagonal range {-69 ... +69}
    if (ux != 0.0 && uy != 0.0) {
        auto slope = uy / ux;
        auto edgex = copysign(MAX_AXIS_RANGE / (fabs(slope) + 16.0 / 69.0), ux);
        auto edgey = copysign(std::min(fabs(edgex * slope), MAX_AXIS_RANGE / (1.0 / fabs(slope) + 16.0 / 69.0)), y);
        edgex = edgey / slope;

        auto scale = sqrt(edgex * edgex + edgey * edgey) / MAX_AXIS_RANGE;
        ux *= scale;
        uy *= scale;
    }

    // map to virtual notches
    const double notchProximityValRadians = mNotchProxmityThreshold * M_TAU / 360;

    const double distance = std::sqrt((ux * ux) + (uy * uy)) / MAX_AXIS_RANGE;
    if (distance >= MINIMUM_RADIUS_TO_MAP_NOTCH) {
        auto angle = atan2(uy, ux) + M_TAU;
        auto newAngle = GetClosestNotch(angle, notchProximityValRadians);

        ux = cos(newAngle) * distance * MAX_AXIS_RANGE;
        uy = sin(newAngle) * distance * MAX_AXIS_RANGE;
    }

    // assign back to original sign
    x = copysign(ux, sx);
    y = copysign(uy, sy);
}

void ControllerStick::UpdateAxisDirectionMapping(Direction direction, std::shared_ptr<AxisDirectionMapping> mapping) {
  switch (direction) {
    case LEFT:
      mLeftMapping = mapping;
      break;
    case RIGHT:
      mRightMapping = mapping;
      break;
    case UP:
      mUpMapping = mapping;
      break;
    case DOWN:
      mDownMapping = mapping;
      break;
  }
}

void ControllerStick::UpdatePad(int8_t& x, int8_t& y) {
  if (mRightMapping == nullptr || mLeftMapping == nullptr || mUpMapping == nullptr || mDownMapping == nullptr) {
    return;
  }

  Process(x, y);
}
} // namespace LUS
