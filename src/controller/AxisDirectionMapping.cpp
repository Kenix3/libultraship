#include "AxisDirectionMapping.h"

namespace LUS {
AxisDirectionMapping::AxisDirectionMapping() {
}

AxisDirectionMapping::~AxisDirectionMapping() {
}

uint8_t AxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_UNKNOWN;
}

std::string AxisDirectionMapping::GetAxisDirectionName() {
    return "Unknown";
}
} // namespace LUS
