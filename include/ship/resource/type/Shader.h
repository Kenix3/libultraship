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
     * @brief Returns a pointer to the shader source string data.
     * @return Pointer to the internal string buffer.
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
