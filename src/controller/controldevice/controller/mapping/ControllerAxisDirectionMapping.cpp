#include "ControllerAxisDirectionMapping.h"

#include <random>
#include <sstream>

namespace LUS {
ControllerAxisDirectionMapping::ControllerAxisDirectionMapping() {
    GenerateUuid();
}

ControllerAxisDirectionMapping::ControllerAxisDirectionMapping(std::string uuid) {
    mUuid = uuid;
}

ControllerAxisDirectionMapping::~ControllerAxisDirectionMapping() {
}

uint8_t ControllerAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_UNKNOWN;
}

std::string ControllerAxisDirectionMapping::GetAxisDirectionName() {
    return "Unknown";
}

std::string ControllerAxisDirectionMapping::GetUuid() {
    return mUuid;
}

void ControllerAxisDirectionMapping::GenerateUuid() {
    // todo: this a better way
    // this is bad multiple reasons, first because it's copypasta from ButtonMapping.cpp
    // the other reasons are explained there
    static std::random_device rd;
    static std::mt19937 gen(rd());
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
