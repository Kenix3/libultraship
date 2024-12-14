#pragma once

namespace Ship {

enum class ResourceType {
    None = 0x00000000,

    Blob = 0x4F424C42, // OBLB
    Json = 0x4A534F4E, // JSON
};
} // namespace Ship

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
