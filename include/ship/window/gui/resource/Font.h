#pragma once

#include "ship/resource/Resource.h"

namespace Ship {

/** @brief Resource type identifier for Font resources (ASCII: "FONT"). */
#define RESOURCE_TYPE_FONT 0x464F4E54 // FONT

/**
 * @brief A resource representing a loaded font file (e.g. TTF).
 *
 * Font wraps raw font data loaded from an archive so that it can be
 * registered with ImGui or another text rendering system.
 */
class Font : public Resource<void> {
  public:
    using Resource::Resource;

    /** @brief Constructs an empty Font resource. */
    Font();

    /** @brief Destructor. */
    virtual ~Font();

    /**
     * @brief Returns a pointer to the raw font data.
     * @return Pointer to the font byte buffer.
     */
    void* GetPointer() override;

    /**
     * @brief Returns the size of the raw font data in bytes.
     * @return Size of the font data buffer.
     */
    size_t GetPointerSize() override;

    /** @brief Raw font file data. */
    char* Data = nullptr;

    /** @brief Size of the font data buffer in bytes. */
    size_t DataSize;
};
}; // namespace Ship
