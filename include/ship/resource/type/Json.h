#pragma once

#include "ship/resource/Resource.h"
#include <nlohmann/json.hpp>

namespace Ship {

/**
 * @brief Resource type representing a parsed JSON document.
 *
 * The JSON data is deserialized into a nlohmann::json object and exposed
 * through the typed Resource interface. Use ResourceType::Json when
 * registering a factory for this type.
 */
class Json final : public Resource<void> {
  public:
    using Resource::Resource;

    /** @brief Default-constructs an empty Json resource. */
    Json();

    /**
     * @brief Returns a pointer to the nlohmann::json data object.
     * @return Pointer to the Data member.
     */
    void* GetPointer() override;

    /**
     * @brief Returns the size of the serialized JSON data in bytes.
     * @return Value of DataSize.
     */
    size_t GetPointerSize() override;

    /** @brief Parsed JSON document. */
    nlohmann::json Data;
    /** @brief Size in bytes of the original serialized JSON string. */
    size_t DataSize;
};
}; // namespace Ship
