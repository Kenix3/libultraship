#include "WheelHandler.h"
#include "Context.h"
#include "spdlog/spdlog.h"

namespace Ship {
WheelHandler::WheelHandler() {
    mDirections = {LUS_WHEEL_NONE, LUS_WHEEL_NONE};
}

WheelHandler::~WheelHandler() {}

std::shared_ptr<WheelHandler> WheelHandler::mInstance;

std::shared_ptr<WheelHandler> WheelHandler::GetInstance() {
    if (mInstance == nullptr) {
        mInstance = std::make_shared<WheelHandler>();
    }
    return mInstance;
}

void WheelHandler::Update() {
    mCoords = Context::GetInstance()->GetWindow()->GetMouseWheel();

    mDirections.x = mDirections.y = LUS_WHEEL_NONE;
    if (mCoords.x < 0) {
        mDirections.x = LUS_WHEEL_LEFT;
    } else if (mCoords.x > 0) {
        mDirections.x = LUS_WHEEL_RIGHT;
    }
    if (mCoords.y < 0) {
        mDirections.y = LUS_WHEEL_DOWN;
    } else if (mCoords.y > 0) {
        mDirections.y = LUS_WHEEL_UP;
    }
    SPDLOG_INFO("WHEEEL: {} {}", mCoords.x, mCoords.y);
}

CoordsF WheelHandler::GetCoords() {
    return mCoords;
}

WheelDirections WheelHandler::GetDirections() {
    return mDirections;
}

float WheelHandler::GetDirectionValue(WheelDirection direction) {
    switch (direction) {
        case LUS_WHEEL_LEFT:
            if (mCoords.x < 0) {
                return -mCoords.x;
            }
            break;
        case LUS_WHEEL_RIGHT:
            if (mCoords.x > 0) {
                return mCoords.x;
            }
            break;
        case LUS_WHEEL_DOWN:
            if (mCoords.y < 0) {
                return -mCoords.y;
            }
            break;
        case LUS_WHEEL_UP:
            if (mCoords.y > 0) {
                return mCoords.y;
            }
    }
    return 0.0f;
}
} // namespace Ship
