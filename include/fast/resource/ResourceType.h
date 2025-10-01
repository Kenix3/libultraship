#pragma once

namespace Fast {

enum class ResourceType {
    None = 0x00000000,

    DisplayList = 0x4F444C54, // ODLT
    Light = 0x46669697,       // LGTS
    Matrix = 0x4F4D5458,      // OMTX
    Texture = 0x4F544558,     // OTEX
    Vertex = 0x4F565458,      // OVTX
};
} // namespace Fast
