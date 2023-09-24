#include "ButtonMapping.h"

#include <random>
#include <sstream>

namespace LUS {
ButtonMapping::ButtonMapping(uint16_t bitmask) : mBitmask(bitmask) {
    GenerateUuid();
}

ButtonMapping::ButtonMapping(uint16_t bitmask, std::string uuid) : mBitmask(bitmask) {
    mUuid = uuid;
}

ButtonMapping::~ButtonMapping() {
}

uint16_t ButtonMapping::GetBitmask() {
    return mBitmask;
}

std::string ButtonMapping::GetUuid() {
    return mUuid;
}

uint8_t ButtonMapping::GetMappingType() {
    return MAPPING_TYPE_UNKNOWN;
}

void ButtonMapping::GenerateUuid() {
    // todo: this a better way
    // i tried some cross-platform uuid lib stuff and ended up fighting cmake
    // so i figured i'd just this for now despite the fact that has been advised
    // against on stackoverflow: https://stackoverflow.com/a/60198074
    static std::random_device              rd;
    static std::mt19937                    gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++) {
        ss << dis(gen);
    };
    mUuid = ss.str();
}
} // namespace LUS