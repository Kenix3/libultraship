#include "SDLButtonMapping.h"

namespace LUS {
SDLButtonMapping::SDLButtonMapping(int32_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerButton)
    : ButtonMapping(bitmask), mControllerIndex(sdlControllerIndex), mControllerButton(sdlControllerButton) {
}

void SDLButtonMapping::UpdatePad(int32_t& padButtons) {
}
} // namespace LUS