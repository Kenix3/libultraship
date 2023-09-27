#include "ControllerStick.h"
#include <spdlog/spdlog.h>

#include "controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToAxisDirectionMapping.h"

#include "public/bridge/consolevariablebridge.h"

#include <Utils/StringHelper.h>

#define M_TAU 6.2831853071795864769252867665590057 // 2 * pi
#define MINIMUM_RADIUS_TO_MAP_NOTCH 0.9

namespace LUS {
ControllerStick::ControllerStick(uint8_t portIndex, Stick stick) : mPortIndex(portIndex), mStick(stick) {
    mDeadzone = 16.0f;
    mNotchProxmityThreshold = 0;
}

ControllerStick::~ControllerStick() {
}

void ControllerStick::ClearAllMappings() {
    for (auto [direction, directionMappings] : mAxisDirectionMappings) {
        for (auto [id, mapping] : directionMappings) {
            mapping->EraseFromConfig();
        }
    }
    mAxisDirectionMappings.clear();
    SaveAxisDirectionMappingIdsToConfig();
}

// todo: where should this live?
std::unordered_map<Stick, std::string> StickToConfigStickName = {
  { LEFT_STICK, "LeftStick" },
  { RIGHT_STICK, "RightStick" }
};

// todo: where should this live?
std::unordered_map<Direction, std::string> DirectionToConfigDirectionName = {
  { LEFT, "Left" },
  { RIGHT, "Right" },
  { UP, "Up" },
  { DOWN, "Down" }
};

void ControllerStick::SaveAxisDirectionMappingIdsToConfig() {
    for (auto [direction, directionMappings] : mAxisDirectionMappings) {
        // todo: this efficently (when we build out cvar array support?)
        std::string axisDirectionMappingIdListString = "";

        for (auto [id, mapping] : directionMappings) {
            axisDirectionMappingIdListString += id;
            axisDirectionMappingIdListString += ",";
        }

        const std::string axisDirectionMappingIdsCvarKey =
            StringHelper::Sprintf("gControllers.Port%d.%s.%sAxisDirectionMappingIds", mPortIndex + 1, StickToConfigStickName[mStick].c_str(),  DirectionToConfigDirectionName[direction].c_str());
        if (axisDirectionMappingIdListString == "") {
            CVarClear(axisDirectionMappingIdsCvarKey.c_str());
        } else {
            CVarSetString(axisDirectionMappingIdsCvarKey.c_str(), axisDirectionMappingIdListString.c_str());
        }
    }

    // for (auto direction : {LEFT, RIGHT, UP, DOWN}) {
    //     const std::string axisDirectionMappingIdsCvarKey =
    //         StringHelper::Sprintf("gControllers.Port%d.%s.%sAxisDirectionMappingIds", mPortIndex + 1, StickToConfigStickName[mStick].c_str(),  DirectionToConfigDirectionName[direction].c_str());
    //     if (axisDirectionMappingIdListString == "") {
    //         CVarClear(axisDirectionMappingIdsCvarKey.c_str());
    //     } else {
    //         CVarSetString(axisDirectionMappingIdsCvarKey.c_str(), axisDirectionMappingIdListString.c_str());
    //     }
    // }
    CVarSave();
}

void ControllerStick::ClearAxisDirectionMapping(Direction direction, std::string id) {
    mAxisDirectionMappings[direction][id]->EraseFromConfig(); 
    mAxisDirectionMappings[direction].erase(id);
    SaveAxisDirectionMappingIdsToConfig();
}

void ControllerStick::ClearAxisDirectionMapping(Direction direction, std::shared_ptr<ControllerAxisDirectionMapping> mapping) {
    ClearAxisDirectionMapping(direction, mapping->GetAxisDirectionMappingId());
}

void ControllerStick::AddAxisDirectionMapping(Direction direction, std::shared_ptr<ControllerAxisDirectionMapping> mapping) {
    mAxisDirectionMappings[direction][mapping->GetAxisDirectionMappingId()] = mapping;
}

void ControllerStick::ResetToDefaultMappings(int32_t sdlControllerIndex) {
    ClearAllMappings();

    AddAxisDirectionMapping(LEFT, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(mPortIndex, mStick, LEFT, sdlControllerIndex, 0, -1));
    AddAxisDirectionMapping(RIGHT, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(mPortIndex, mStick, RIGHT, sdlControllerIndex, 0, 1));
    AddAxisDirectionMapping(UP, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(mPortIndex, mStick, UP, sdlControllerIndex, 1, -1));
    AddAxisDirectionMapping(DOWN, std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(mPortIndex, mStick, DOWN, sdlControllerIndex, 1, 1));

    for (auto [direction, directionMappings] : mAxisDirectionMappings) {
        for (auto [id, mapping] : directionMappings) {
            mapping->SaveToConfig();
        }
    }
    SaveAxisDirectionMappingIdsToConfig();
}

void ControllerStick::LoadAxisDirectionMappingFromConfig(std::string id) {
    // todo: maybe this stuff makes sense in a factory?
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(), "");

    if (mappingClass == "SDLAxisDirectionToAxisDirectionMapping") {
        int32_t direction = CVarGetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), 0);
        int32_t sdlControllerAxis =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), 0);
        int32_t axisDirection =
            CVarGetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), 0);

        if ((direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) || sdlControllerIndex < 0 || sdlControllerAxis == -1 ||
            (axisDirection != NEGATIVE && axisDirection != POSITIVE)) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return;
        }

        AddAxisDirectionMapping(static_cast<Direction>(direction), std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(mPortIndex, mStick, static_cast<Direction>(direction), sdlControllerIndex, sdlControllerAxis, axisDirection));
        return;
    }
}

void ControllerStick::ReloadAllMappingsFromConfig() {
    mAxisDirectionMappings.clear();

    // todo: this efficently (when we build out cvar array support?)
    // i don't expect it to really be a problem with the small number of mappings we have
    // for each controller (especially compared to include/exclude locations in rando), and
    // the audio editor pattern doesn't work for this because that looks for ids that are either
    // hardcoded or provided by an otr file
    for (auto direction : {LEFT, RIGHT, UP, DOWN}) {
        const std::string axisDirectionMappingIdsCvarKey =
            StringHelper::Sprintf("gControllers.Port%d.%s.%sAxisDirectionMappingIds", mPortIndex + 1, StickToConfigStickName[mStick].c_str(),  DirectionToConfigDirectionName[direction].c_str());

        std::stringstream axisDirectionMappingIdsStringStream(CVarGetString(axisDirectionMappingIdsCvarKey.c_str(), ""));
        std::string axisDirectionMappingIdString;
        while (getline(axisDirectionMappingIdsStringStream, axisDirectionMappingIdString, ',')) {
            LoadAxisDirectionMappingFromConfig(axisDirectionMappingIdString);
        }
    }
}

double ControllerStick::GetClosestNotch(double angle, double approximationThreshold) {
    constexpr auto octagonAngle = M_TAU / 8;
    const auto closestNotch = std::round(angle / octagonAngle) * octagonAngle;
    const auto distanceToNotch = std::abs(fmod(closestNotch - angle + M_PI, M_TAU) - M_PI);
    return distanceToNotch < approximationThreshold / 2 ? closestNotch : angle;
}

float ControllerStick::GetAxisDirectionValue(Direction direction) {
    if (mAxisDirectionMappings[direction].size() == 0) {
        return 0;
    }

    if (mAxisDirectionMappings[direction].size() == 1) {
        return mAxisDirectionMappings[direction].begin()->second->GetNormalizedAxisDirectionValue();
    }

    auto maxMapping = std::max_element(mAxisDirectionMappings[direction].begin(), mAxisDirectionMappings[direction].end(), [](const std::pair<std::string, std::shared_ptr<ControllerAxisDirectionMapping>> a, const std::pair<std::string, std::shared_ptr<ControllerAxisDirectionMapping>> b) {
        return a.second->GetNormalizedAxisDirectionValue() < b.second->GetNormalizedAxisDirectionValue();
    });
    return maxMapping->second->GetNormalizedAxisDirectionValue();
}

void ControllerStick::Process(int8_t& x, int8_t& y) {
    // auto sx = mRightMapping->GetNormalizedAxisDirectionValue() - mLeftMapping->GetNormalizedAxisDirectionValue();
    // auto sy = mUpMapping->GetNormalizedAxisDirectionValue() - mDownMapping->GetNormalizedAxisDirectionValue();

    auto sx = GetAxisDirectionValue(RIGHT) - GetAxisDirectionValue(LEFT);
    auto sy = GetAxisDirectionValue(UP) - GetAxisDirectionValue(DOWN);

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

std::shared_ptr<ControllerAxisDirectionMapping> ControllerStick::GetAxisDirectionMappingByDirection(Direction direction) {
    // todo: actually make the multimap stuff work here
    return mAxisDirectionMappings[direction].begin()->second;
}

void ControllerStick::UpdatePad(int8_t& x, int8_t& y) {
    Process(x, y);
}
} // namespace LUS
