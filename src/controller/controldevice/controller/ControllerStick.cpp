#include "ControllerStick.h"
#include <spdlog/spdlog.h>

#include "controller/controldevice/controller/mapping/keyboard/KeyboardKeyToAxisDirectionMapping.h"

#include "controller/controldevice/controller/mapping/factories/AxisDirectionMappingFactory.h"

#include "public/bridge/consolevariablebridge.h"

#include "utils/StringHelper.h"
#include <sstream>
#include <algorithm>

// for some reason windows isn't seeing M_PI
// this is copied from my system's math.h
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define M_TAU 2 * M_PI
#define MINIMUM_RADIUS_TO_MAP_NOTCH 0.9

namespace Ship {
ControllerStick::ControllerStick(uint8_t portIndex, Stick stick)
    : mPortIndex(portIndex), mStick(stick), mUseKeydownEventToCreateNewMapping(false),
      mKeyboardScancodeForNewMapping(KbScancode::LUS_KB_UNKNOWN) {
    mSensitivityPercentage = DEFAULT_STICK_SENSITIVITY_PERCENTAGE;
    mSensitivity = 1.0f;
    mDeadzonePercentage = DEFAULT_STICK_DEADZONE_PERCENTAGE;
    mDeadzone = 17.0f;
    mNotchSnapAngle = 0;
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
    SetDeadzone(20);
    SetNotchSnapAngle(0);
}

void ControllerStick::ClearAllMappingsForDevice(ShipDeviceIndex lusIndex) {
    std::vector<std::string> mappingIdsToRemove;
    for (auto [direction, directionMappings] : mAxisDirectionMappings) {
        for (auto [id, mapping] : directionMappings) {
            if (mapping->GetShipDeviceIndex() == lusIndex) {
                mapping->EraseFromConfig();
                mappingIdsToRemove.push_back(id);
            }
        }
    }

    for (auto id : mappingIdsToRemove) {
        for (auto [direction, directionMappings] : mAxisDirectionMappings) {
            auto it = directionMappings.find(id);
            if (it != directionMappings.end()) {
                directionMappings.erase(it);
            }
        }
    }
    SaveAxisDirectionMappingIdsToConfig();
}

// todo: where should this live?
std::unordered_map<Stick, std::string> stickToConfigStickName = { { LEFT_STICK, "LeftStick" },
                                                                  { RIGHT_STICK, "RightStick" } };

// todo: where should this live?
std::unordered_map<Direction, std::string> directionToConfigDirectionName = {
    { LEFT, "Left" }, { RIGHT, "Right" }, { UP, "Up" }, { DOWN, "Down" }
};

void ControllerStick::SaveAxisDirectionMappingIdsToConfig() {
    for (auto [direction, directionMappings] : mAxisDirectionMappings) {
        // todo: this efficently (when we build out cvar array support?)
        std::string axisDirectionMappingIdListString = "";

        for (auto [id, mapping] : directionMappings) {
            axisDirectionMappingIdListString += id;
            axisDirectionMappingIdListString += ",";
        }

        const std::string axisDirectionMappingIdsCvarKey = StringHelper::Sprintf(
            CVAR_PREFIX_CONTROLLERS ".Port%d.%s.%sAxisDirectionMappingIds", mPortIndex + 1,
            stickToConfigStickName[mStick].c_str(), directionToConfigDirectionName[direction].c_str());
        if (axisDirectionMappingIdListString == "") {
            CVarClear(axisDirectionMappingIdsCvarKey.c_str());
        } else {
            CVarSetString(axisDirectionMappingIdsCvarKey.c_str(), axisDirectionMappingIdListString.c_str());
        }
    }

    CVarSave();
}

void ControllerStick::ClearAxisDirectionMappingId(Direction direction, std::string id) {
    mAxisDirectionMappings[direction].erase(id);
    SaveAxisDirectionMappingIdsToConfig();
}

void ControllerStick::ClearAxisDirectionMapping(Direction direction, std::string id) {
    mAxisDirectionMappings[direction][id]->EraseFromConfig();
    mAxisDirectionMappings[direction].erase(id);
    SaveAxisDirectionMappingIdsToConfig();
}

void ControllerStick::ClearAxisDirectionMapping(Direction direction,
                                                std::shared_ptr<ControllerAxisDirectionMapping> mapping) {
    ClearAxisDirectionMapping(direction, mapping->GetAxisDirectionMappingId());
}

void ControllerStick::AddAxisDirectionMapping(Direction direction,
                                              std::shared_ptr<ControllerAxisDirectionMapping> mapping) {
    mAxisDirectionMappings[direction][mapping->GetAxisDirectionMappingId()] = mapping;
}

void ControllerStick::AddDefaultMappings(ShipDeviceIndex lusIndex) {
    for (auto mapping :
         AxisDirectionMappingFactory::CreateDefaultSDLAxisDirectionMappings(lusIndex, mPortIndex, mStick)) {
        AddAxisDirectionMapping(mapping->GetDirection(), mapping);
    }

    if (lusIndex == ShipDeviceIndex::Keyboard) {
        for (auto mapping :
             AxisDirectionMappingFactory::CreateDefaultKeyboardAxisDirectionMappings(mPortIndex, mStick)) {
            AddAxisDirectionMapping(mapping->GetDirection(), mapping);
        }
    }

    for (auto [direction, directionMappings] : mAxisDirectionMappings) {
        for (auto [id, mapping] : directionMappings) {
            mapping->SaveToConfig();
        }
    }
    SaveAxisDirectionMappingIdsToConfig();
}

void ControllerStick::LoadAxisDirectionMappingFromConfig(std::string id) {
    auto mapping = AxisDirectionMappingFactory::CreateAxisDirectionMappingFromConfig(mPortIndex, mStick, id);

    if (mapping == nullptr) {
        return;
    }

    AddAxisDirectionMapping(mapping->GetDirection(), mapping);
}

void ControllerStick::ReloadAllMappingsFromConfig() {
    mAxisDirectionMappings.clear();

    // todo: this efficently (when we build out cvar array support?)
    // i don't expect it to really be a problem with the small number of mappings we have
    // for each controller (especially compared to include/exclude locations in rando), and
    // the audio editor pattern doesn't work for this because that looks for ids that are either
    // hardcoded or provided by an otr file
    for (auto direction : { LEFT, RIGHT, UP, DOWN }) {
        const std::string axisDirectionMappingIdsCvarKey = StringHelper::Sprintf(
            CVAR_PREFIX_CONTROLLERS ".Port%d.%s.%sAxisDirectionMappingIds", mPortIndex + 1,
            stickToConfigStickName[mStick].c_str(), directionToConfigDirectionName[direction].c_str());

        std::stringstream axisDirectionMappingIdsStringStream(
            CVarGetString(axisDirectionMappingIdsCvarKey.c_str(), ""));
        std::string axisDirectionMappingIdString;
        while (getline(axisDirectionMappingIdsStringStream, axisDirectionMappingIdString, ',')) {
            LoadAxisDirectionMappingFromConfig(axisDirectionMappingIdString);
        }
    }

    SetSensitivity(CVarGetInteger(StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.%s.SensitivityPercentage",
                                                        mPortIndex + 1, stickToConfigStickName[mStick].c_str())
                                      .c_str(),
                                  DEFAULT_STICK_SENSITIVITY_PERCENTAGE));

    SetDeadzone(CVarGetInteger(StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.%s.DeadzonePercentage",
                                                     mPortIndex + 1, stickToConfigStickName[mStick].c_str())
                                   .c_str(),
                               DEFAULT_STICK_DEADZONE_PERCENTAGE));

    SetNotchSnapAngle(CVarGetInteger(StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.%s.NotchSnapAngle",
                                                           mPortIndex + 1, stickToConfigStickName[mStick].c_str())
                                         .c_str(),
                                     0));
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

    auto maxMapping = std::max_element(
        mAxisDirectionMappings[direction].begin(), mAxisDirectionMappings[direction].end(),
        [](const std::pair<std::string, std::shared_ptr<ControllerAxisDirectionMapping>> a,
           const std::pair<std::string, std::shared_ptr<ControllerAxisDirectionMapping>> b) {
            return a.second->GetNormalizedAxisDirectionValue() < b.second->GetNormalizedAxisDirectionValue();
        });
    return maxMapping->second->GetNormalizedAxisDirectionValue();
}

void ControllerStick::Process(int8_t& x, int8_t& y) {
    double sx = GetAxisDirectionValue(RIGHT) - GetAxisDirectionValue(LEFT);
    double sy = GetAxisDirectionValue(UP) - GetAxisDirectionValue(DOWN);

    double ux = fabs(sx) * mSensitivity;
    double uy = fabs(sy) * mSensitivity;

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
    const double notchProximityValRadians = mNotchSnapAngle * M_TAU / 360;

    const double distance = std::sqrt((ux * ux) + (uy * uy)) / MAX_AXIS_RANGE;
    if (distance >= MINIMUM_RADIUS_TO_MAP_NOTCH) {
        auto angle = std::atan2(uy, ux) + M_TAU;
        auto newAngle = GetClosestNotch(angle, notchProximityValRadians);

        ux = std::cos(newAngle) * distance * MAX_AXIS_RANGE;
        uy = std::sin(newAngle) * distance * MAX_AXIS_RANGE;
    }

    // assign back to original sign
    x = copysign(ux, sx);
    y = copysign(uy, sy);
}

bool ControllerStick::AddOrEditAxisDirectionMappingFromRawPress(Direction direction, std::string id) {
    std::shared_ptr<ControllerAxisDirectionMapping> mapping = nullptr;

    mUseKeydownEventToCreateNewMapping = true;
    if (mKeyboardScancodeForNewMapping != LUS_KB_UNKNOWN) {
        mapping = std::make_shared<KeyboardKeyToAxisDirectionMapping>(mPortIndex, mStick, direction,
                                                                      mKeyboardScancodeForNewMapping);
    }

    if (mapping == nullptr) {
        mapping = AxisDirectionMappingFactory::CreateAxisDirectionMappingFromSDLInput(mPortIndex, mStick, direction);
    }

    if (mapping == nullptr) {
        return false;
    }

    mKeyboardScancodeForNewMapping = LUS_KB_UNKNOWN;
    mUseKeydownEventToCreateNewMapping = false;

    if (id != "") {
        ClearAxisDirectionMapping(direction, id);
    }

    AddAxisDirectionMapping(direction, mapping);
    mapping->SaveToConfig();
    SaveAxisDirectionMappingIdsToConfig();
    const std::string hasConfigCvarKey =
        StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.HasConfig", mPortIndex + 1);
    CVarSetInteger(hasConfigCvarKey.c_str(), true);
    CVarSave();
    return true;
}

std::shared_ptr<ControllerAxisDirectionMapping> ControllerStick::GetAxisDirectionMappingById(Direction direction,
                                                                                             std::string id) {
    if (!mAxisDirectionMappings[direction].contains(id)) {
        return nullptr;
    }

    return mAxisDirectionMappings[direction][id];
}

std::unordered_map<std::string, std::shared_ptr<ControllerAxisDirectionMapping>>
ControllerStick::GetAllAxisDirectionMappingByDirection(Direction direction) {
    return mAxisDirectionMappings[direction];
}

void ControllerStick::UpdatePad(int8_t& x, int8_t& y) {
    Process(x, y);
}

bool ControllerStick::ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode) {
    if (mUseKeydownEventToCreateNewMapping && eventType == LUS_KB_EVENT_KEY_DOWN) {
        mKeyboardScancodeForNewMapping = scancode;
        return true;
    }

    bool result = false;
    for (auto [direction, mappings] : mAxisDirectionMappings) {
        for (auto [id, mapping] : mappings) {
            if (mapping->GetMappingType() == MAPPING_TYPE_KEYBOARD) {
                std::shared_ptr<KeyboardKeyToAxisDirectionMapping> ktoadMapping =
                    std::dynamic_pointer_cast<KeyboardKeyToAxisDirectionMapping>(mapping);
                if (ktoadMapping != nullptr) {
                    result = result || ktoadMapping->ProcessKeyboardEvent(eventType, scancode);
                }
            }
        }
    }
    return result;
}

void ControllerStick::SetSensitivity(uint8_t sensitivityPercentage) {
    mSensitivityPercentage = sensitivityPercentage;
    mSensitivity = sensitivityPercentage / 100.0f;
    CVarSetInteger(StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.%s.SensitivityPercentage", mPortIndex + 1,
                                         stickToConfigStickName[mStick].c_str())
                       .c_str(),
                   mSensitivityPercentage);
    CVarSave();
}

void ControllerStick::ResetSensitivityToDefault() {
    SetSensitivity(DEFAULT_STICK_SENSITIVITY_PERCENTAGE);
}

uint8_t ControllerStick::GetSensitivityPercentage() {
    return mSensitivityPercentage;
}

bool ControllerStick::SensitivityIsDefault() {
    return mSensitivityPercentage == DEFAULT_STICK_SENSITIVITY_PERCENTAGE;
}

void ControllerStick::SetDeadzone(uint8_t deadzonePercentage) {
    mDeadzonePercentage = deadzonePercentage;
    mDeadzone = MAX_AXIS_RANGE * (deadzonePercentage / 100.0f);
    CVarSetInteger(StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.%s.DeadzonePercentage", mPortIndex + 1,
                                         stickToConfigStickName[mStick].c_str())
                       .c_str(),
                   mDeadzonePercentage);
    CVarSave();
}

void ControllerStick::ResetDeadzoneToDefault() {
    SetDeadzone(DEFAULT_STICK_DEADZONE_PERCENTAGE);
}

uint8_t ControllerStick::GetDeadzonePercentage() {
    return mDeadzonePercentage;
}

bool ControllerStick::DeadzoneIsDefault() {
    return mDeadzonePercentage == DEFAULT_STICK_DEADZONE_PERCENTAGE;
}

void ControllerStick::SetNotchSnapAngle(uint8_t notchSnapAngle) {
    mNotchSnapAngle = notchSnapAngle;
    CVarSetInteger(StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.%s.NotchSnapAngle", mPortIndex + 1,
                                         stickToConfigStickName[mStick].c_str())
                       .c_str(),
                   mNotchSnapAngle);
    CVarSave();
}

void ControllerStick::ResetNotchSnapAngleToDefault() {
    SetNotchSnapAngle(DEFAULT_NOTCH_SNAP_ANGLE);
}

uint8_t ControllerStick::GetNotchSnapAngle() {
    return mNotchSnapAngle;
}

bool ControllerStick::NotchSnapAngleIsDefault() {
    return mNotchSnapAngle == DEFAULT_NOTCH_SNAP_ANGLE;
}

bool ControllerStick::HasMappingsForShipDeviceIndex(ShipDeviceIndex lusIndex) {
    return std::any_of(mAxisDirectionMappings.begin(), mAxisDirectionMappings.end(),
                       [lusIndex](const auto& directionMappings) {
                           return std::any_of(directionMappings.second.begin(), directionMappings.second.end(),
                                              [lusIndex](const auto& mappingPair) {
                                                  return mappingPair.second->GetShipDeviceIndex() == lusIndex;
                                              });
                       });
}

std::unordered_map<Direction, std::unordered_map<std::string, std::shared_ptr<ControllerAxisDirectionMapping>>>
ControllerStick::GetAllAxisDirectionMappings() {
    return mAxisDirectionMappings;
}

Stick ControllerStick::LeftOrRightStick() {
    return mStick;
}
} // namespace Ship
