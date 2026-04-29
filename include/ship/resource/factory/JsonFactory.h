#pragma once

#include "ship/resource/Resource.h"
#include "ship/resource/ResourceFactoryBinary.h"

namespace Ship {
/**
 * @brief Binary factory for Json resources (version 0).
 *
 * Reads a JSON string from a File's BinaryReader, parses it, and produces a Json resource.
 */
class ResourceFactoryBinaryJsonV0 final : public ResourceFactoryBinary {
  public:
    /**
     * @brief Deserializes a binary file into a Json resource.
     * @param file     Raw file data with a BinaryReader.
     * @param initData Metadata parsed from the file header.
     * @return Shared pointer to the constructed Json, or nullptr on failure.
     */
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file,
                                            std::shared_ptr<Ship::ResourceInitData> initData) override;
};
}; // namespace Ship
