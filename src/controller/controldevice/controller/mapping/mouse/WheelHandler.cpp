#include "WheelHandler.h"
#include "Context.h"
#include <cmath>

namespace Ship {
WheelHandler::WheelHandler() {
    mDirections = { LUS_WHEEL_NONE, LUS_WHEEL_NONE };
}

WheelHandler::~WheelHandler() {
}

std::shared_ptr<WheelHandler> WheelHandler::mInstance;

std::shared_ptr<WheelHandler> WheelHandler::GetInstance() {
    if (mInstance == nullptr) {
        mInstance = std::make_shared<WheelHandler>();
    }
    return mInstance;
}

void WheelHandler::UpdateAxisBuffer(float* buf, float input) {
    static const float LIMIT = 3.0f;
    static const float REDUCE_STEP = 1.0f;

    if (input != 0.0f) {
        // add current input to buffer
        *buf += input;
        // limit buffer
        if (fabs(*buf) > LIMIT) {
            *buf = copysignf(LIMIT, *buf);
        }
    } else if (*buf != 0.0f) {
        // reduce buffered value
        if (fabs(*buf) <= REDUCE_STEP) {
            *buf = 0.0f;
        } else {
            *buf -= copysignf(REDUCE_STEP, *buf);
        }
    }
}

void WheelHandler::Update() {
    mCoords = Context::GetInstance()->GetWindow()->GetMouseWheel();

    UpdateAxisBuffer(&mBufferedCoords.x, mCoords.x);
    UpdateAxisBuffer(&mBufferedCoords.y, mCoords.y);

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
}

CoordsF WheelHandler::GetCoords() {
    return mCoords;
}

WheelDirections WheelHandler::GetDirections() {
    return mDirections;
}

float WheelHandler::CalcDirectionValue(CoordsF& coords, WheelDirection direction) {
    switch (direction) {
        case LUS_WHEEL_LEFT:
            if (coords.x < 0) {
                return -coords.x;
            }
            break;
        case LUS_WHEEL_RIGHT:
            if (coords.x > 0) {
                return coords.x;
            }
            break;
        case LUS_WHEEL_DOWN:
            if (coords.y < 0) {
                return -coords.y;
            }
            break;
        case LUS_WHEEL_UP:
            if (coords.y > 0) {
                return coords.y;
            }
    }
    return 0.0f;
}

float WheelHandler::GetDirectionValue(WheelDirection direction) {
    return CalcDirectionValue(mCoords, direction);
}

float WheelHandler::GetBufferedDirectionValue(WheelDirection direction) {
    return CalcDirectionValue(mBufferedCoords, direction);
}
} // namespace Ship
