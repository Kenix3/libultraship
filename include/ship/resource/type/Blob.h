#pragma once

#include "ship/resource/Resource.h"

namespace Ship {
/**
 * @brief Resource type representing a generic binary blob.
 *
 * The raw bytes are stored in the Data vector and exposed through the
 * typed Resource interface. Use ResourceType::Blob when registering a
 * factory for this type.
 */
class Blob final : public Ship::Resource<void> {
  public:
    using Resource::Resource;

    /** @brief Default-constructs an empty Blob resource. */
    Blob();

    /**
     * @brief Returns a pointer to the start of the raw byte data.
     * @return Pointer to the first element of Data, or nullptr if empty.
     */
    void* GetPointer() override;

    /**
     * @brief Returns the size in bytes of the blob data.
     * @return Number of bytes in Data.
     */
    size_t GetPointerSize() override;

    /** @brief Raw binary payload. */
    std::vector<uint8_t> Data;
};
}; // namespace Ship
