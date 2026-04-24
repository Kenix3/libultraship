#pragma once

#include "ship/resource/Resource.h"
#include "ship/resource/type/Shader.h"
#include "ship/resource/ResourceFactoryBinary.h"

namespace Ship {
/**
 * @brief Binary factory for Shader resources (version 0).
 *
 * Reads shader source code from a File's BinaryReader and produces a Shader resource.
 */
class ResourceFactoryBinaryShaderV0 final : public ResourceFactoryBinary {
  public:
    /**
     * @brief Deserializes a binary file into a Shader resource.
     * @param file     Raw file data with a BinaryReader.
     * @param initData Metadata parsed from the file header.
     * @return Shared pointer to the constructed Shader, or nullptr on failure.
     */
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file,
                                            std::shared_ptr<Ship::ResourceInitData> initData) override;
};
}; // namespace Ship