#pragma once

namespace Ship {

enum class ResourceType {
    None = 0x00000000,

    Blob = 0x4F424C42,   // OBLB
    Json = 0x4A534F4E,   // JSON
    Shader = 0x53484144, // SHAD
};
} // namespace Ship
