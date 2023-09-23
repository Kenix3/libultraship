#include "SDLAxisDirectionToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>

#define MAX_SDL_RANGE (float)INT16_MAX

namespace LUS {
SDLAxisDirectionToAxisDirectionMapping::SDLAxisDirectionToAxisDirectionMapping(int32_t sdlControllerIndex,
                                                                               int32_t sdlControllerAxis,
                                                                               int32_t axisDirection)
    : SDLMapping(sdlControllerIndex) {
    mControllerAxis = static_cast<SDL_GameControllerAxis>(sdlControllerAxis);
    mAxisDirection = static_cast<AxisDirection>(axisDirection);
}

float SDLAxisDirectionToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (!ControllerLoaded()) {
        return 0.0f;
    }

    const auto axisValue = SDL_GameControllerGetAxis(mController, mControllerAxis);

    if ((mAxisDirection == POSITIVE && axisValue < 0) || (mAxisDirection == NEGATIVE && axisValue > 0)) {
        return 0.0f;
    }

    // scale {-32768 ... +32767} to {-MAX_AXIS_RANGE ... +MAX_AXIS_RANGE}
    // and return the absolute value of it
    return fabs(axisValue * MAX_AXIS_RANGE / MAX_SDL_RANGE);
}
} // namespace LUS
