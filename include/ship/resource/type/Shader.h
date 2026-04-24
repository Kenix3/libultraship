#pragma once

#include "ship/resource/Resource.h"
namespace Ship {

/**
 * @brief Resource type representing a shader program source.
 *
 * The shader source code is stored as a string and exposed through
 * the typed Resource interface. Use ResourceType::Shader when
 * registering a factory for this type.
 */
class Shader final : public Resource<void> {
  public:
    using Resource::Resource;

    /** @brief Default-constructs an empty Shader resource. */
    Shader();

    /**
     * @brief Returns a pointer to the shader source std::string object.
     * @return Pointer to the internal @c std::string field (@c &Data).
     *         To access the raw character buffer, call @c GetPointer() and
     *         cast the result to <tt>std::string*</tt>, then call @c data().
     */
    void* GetPointer() override;

    /**
     * @brief Returns the size in bytes of the shader source string.
     * @return Length of Data.
     */
    size_t GetPointerSize() override;

    /** @brief Shader source code. */
    std::string Data;
};
}; // namespace Ship
