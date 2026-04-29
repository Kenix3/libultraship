#pragma once

#include "ship/resource/Resource.h"
#include "ship/resource/ResourceFactoryBinary.h"

namespace Ship {
/**
 * @brief Binary factory for Blob resources (version 0).
 *
 * Reads a raw binary blob from a File's BinaryReader and produces a Blob resource.
 */
class ResourceFactoryBinaryBlobV0 final : public Ship::ResourceFactoryBinary {
  public:
    /**
     * @brief Deserializes a binary file into a Blob resource.
     * @param file     Raw file data with a BinaryReader.
     * @param initData Metadata parsed from the file header.
     * @return Shared pointer to the constructed Blob, or nullptr on failure.
     */
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;
};
}; // namespace Ship
