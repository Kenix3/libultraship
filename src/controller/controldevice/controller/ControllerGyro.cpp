#include "ControllerGyro.h"

namespace LUS {
ControllerGyro::ControllerGyro() {
}

ControllerGyro::~ControllerGyro() {
}

std::shared_ptr<GyroAxisMapping> ControllerGyro::GetGyroXMapping() {
    return mGyroXMapping;
}

std::shared_ptr<GyroAxisMapping> ControllerGyro::GetGyroYMapping() {
    return mGyroYMapping;
}

void ControllerGyro::UpdatePad(float& x, float& y) {
    if (mGyroXMapping == nullptr || mGyroYMapping == nullptr) {
        return;
    }

    x = GetGyroXMapping()->GetGyroAxisValue();
    y = GetGyroYMapping()->GetGyroAxisValue();
}

} // namespace LUS
