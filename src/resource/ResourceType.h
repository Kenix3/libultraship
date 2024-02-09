#pragma once

namespace LUS {

enum class ResourceType {
    // Not set
    None = 0x00000000,

    // Common
    Archive = 0x4F415243,     // OARC (UNUSED)
    DisplayList = 0x4F444C54, // ODLT
    Vertex = 0x4F565458,      // OVTX
    Matrix = 0x4F4D5458,      // OMTX
    Array = 0x4F415252,       // OARR
    Blob = 0x4F424C42,        // OBLB
    Texture = 0x4F544558,     // OTEX
};
} // namespace LUS
