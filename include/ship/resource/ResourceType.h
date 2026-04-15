#pragma once

namespace Ship {

/**
 * @brief Numeric type identifiers for built-in resource types.
 *
 * Each enumerator's underlying value is the four-character ASCII code of the
 * type as a big-endian 32-bit integer, making the value human-readable in a
 * hex dump (e.g. Blob == 'OBLB').
 *
 * Custom resource types should use values outside the Ship-reserved range
 * and register them with ResourceLoader::RegisterResourceFactory().
 */
enum class ResourceType {
    None = 0x00000000,

    Blob = 0x4F424C42,   ///< Generic binary blob ("OBLB")
    Json = 0x4A534F4E,   ///< JSON document ("JSON")
    Shader = 0x53484144, ///< Shader program ("SHAD")
};
} // namespace Ship
