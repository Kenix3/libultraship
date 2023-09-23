#include "ButtonMapping.h"

namespace LUS {
ButtonMapping::ButtonMapping(uint16_t bitmask) : mBitmask(bitmask) {
}

ButtonMapping::~ButtonMapping() {
}

uint16_t ButtonMapping::GetBitmask() {
    return mBitmask;
}
} // namespace LUS