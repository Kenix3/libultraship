#include "ControllerStick.h"
#include <spdlog/spdlog.h>

#include "controller/sdl/SDLAxisDirectionToAxisDirectionMapping.h"
#include "public/bridge/consolevariablebridge.h"

#include <Utils/StringHelper.h>

#define M_TAU 6.2831853071795864769252867665590057 // 2 * pi
#define MINIMUM_RADIUS_TO_MAP_NOTCH 0.9

namespace LUS {
ControllerStick::ControllerStick(Stick stick) : mStick(stick) {
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

void ControllerStick::SaveToConfig(uint8_t port) {
    const std::string stickCvarKey = StringHelper::Sprintf("gControllers.Port%d.%s", port + 1, mStick == LEFT_STICK ? "LeftStick" : "RightStick");
    CVarSetString(StringHelper::Sprintf("%s.LeftMappingId", stickCvarKey.c_str()).c_str(), mLeftMapping->GetUuid().c_str());
    CVarSetString(StringHelper::Sprintf("%s.RightMappingId", stickCvarKey.c_str()).c_str(), mRightMapping->GetUuid().c_str());
    CVarSetString(StringHelper::Sprintf("%s.UpMappingId", stickCvarKey.c_str()).c_str(), mUpMapping->GetUuid().c_str());
    CVarSetString(StringHelper::Sprintf("%s.DownMappingId", stickCvarKey.c_str()).c_str(), mDownMapping->GetUuid().c_str());
    CVarSave();
}

void ControllerStick::ResetToDefaultMappings(uint8_t port, int32_t sdlControllerIndex) {
    ClearAllMappings();

    UpdateAxisDirectionMapping(LEFT, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(sdlControllerIndex, 0, -1));
    mLeftMapping->SaveToConfig();
    UpdateAxisDirectionMapping(RIGHT, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(sdlControllerIndex, 0, 1));
    mRightMapping->SaveToConfig();
    UpdateAxisDirectionMapping(UP, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(sdlControllerIndex, 1, -1));
    mUpMapping->SaveToConfig();
    UpdateAxisDirectionMapping(DOWN, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(sdlControllerIndex, 1, 1));
    mDownMapping->SaveToConfig();

    SaveToConfig(port);
}

void ControllerStick::LoadAxisDirectionMappingFromConfig(std::string uuid, Direction direction) {
    // todo: maybe this stuff makes sense in a factory?
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + uuid;
    const std::string mappingClass = CVarGetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(), "");

    if (mappingClass == "SDLAxisDirectionToAxisDirectionMapping") {
        int32_t sdlControllerIndex = CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), 0);
        int32_t sdlControllerAxis = CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), 0);
        int32_t axisDirection = CVarGetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), 0);
        
        if (sdlControllerIndex < 0 || sdlControllerAxis == -1 || (axisDirection != NEGATIVE && axisDirection != POSITIVE)) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return;
        }
        
        UpdateAxisDirectionMapping(direction, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(uuid, sdlControllerIndex, sdlControllerAxis, axisDirection));
        return;
    }
}

void ControllerStick::ReloadAllMappingsFromConfig(uint8_t port) {
    ClearAllMappings();

    const std::string stickCvarKey = StringHelper::Sprintf("gControllers.Port%d.%s", port + 1, mStick == LEFT_STICK ? "LeftStick" : "RightStick");
    std::string leftUuidString = CVarGetString(StringHelper::Sprintf("%s.LeftMappingId", stickCvarKey.c_str()).c_str(), "");
    std::string rightUuidString = CVarGetString(StringHelper::Sprintf("%s.RightMappingId", stickCvarKey.c_str()).c_str(), "");
    std::string upUuidString = CVarGetString(StringHelper::Sprintf("%s.UpMappingId", stickCvarKey.c_str()).c_str(), "");
    std::string downUuidString = CVarGetString(StringHelper::Sprintf("%s.DownMappingId", stickCvarKey.c_str()).c_str(), "");
    if (leftUuidString != "") {
      LoadAxisDirectionMappingFromConfig(leftUuidString, LEFT);
    }
    if (rightUuidString != "") {
      LoadAxisDirectionMappingFromConfig(rightUuidString, RIGHT);
    }
    if (upUuidString != "") {
      LoadAxisDirectionMappingFromConfig(upUuidString, UP);
    }
    if (downUuidString != "") {
      LoadAxisDirectionMappingFromConfig(downUuidString, DOWN);
    }
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

std::shared_ptr<AxisDirectionMapping> ControllerStick::GetAxisDirectionMappingByDirection(Direction direction) {
  switch (direction) {
    case LEFT:
      return mLeftMapping;
    case RIGHT:
      return mRightMapping;
    case UP:
      return mUpMapping;
    case DOWN:
      return mDownMapping;
  }
}

void ControllerStick::UpdatePad(int8_t& x, int8_t& y) {
  if (mRightMapping == nullptr || mLeftMapping == nullptr || mUpMapping == nullptr || mDownMapping == nullptr) {
    return;
  }

  Process(x, y);
}
} // namespace LUS
