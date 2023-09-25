#include "AxisDirectionMapping.h"

#include <random>
#include <sstream>

namespace LUS {
AxisDirectionMapping::AxisDirectionMapping() {
    GenerateUuid();
}

AxisDirectionMapping::AxisDirectionMapping(std::string uuid) {
    mUuid = uuid;
}


AxisDirectionMapping::~AxisDirectionMapping() {
}

uint8_t AxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_UNKNOWN;
}

std::string AxisDirectionMapping::GetAxisDirectionName() {
    return "Unknown";
}

std::string AxisDirectionMapping::GetUuid() {
    return mUuid;
}

void AxisDirectionMapping::GenerateUuid() {
    // todo: this a better way
    // this is bad multiple reasons, first because it's copypasta from ButtonMapping.cpp
    // the other reasons are explained there
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
