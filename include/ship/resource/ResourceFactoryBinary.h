#pragma once

#include <ship/resource/ResourceFactory.h>

namespace Ship {
/**
 * @brief ResourceFactory specialization for binary-format resource files.
 *
 * Validates that the file carries a BinaryReader and that its format tag equals
 * RESOURCE_FORMAT_BINARY. Subclass this when implementing a factory that
 * deserializes a binary (.otr / .o2r) resource.
 */
class ResourceFactoryBinary : public ResourceFactory {
  protected:
    /**
     * @brief Validates that the file uses the binary format and contains a BinaryReader.
     * @param file     Raw file data to inspect.
     * @param initData Metadata from the file header.
     * @return true if the format is binary and the reader is a BinaryReader.
     */
    bool FileHasValidFormatAndReader(std::shared_ptr<File> file,
                                     std::shared_ptr<Ship::ResourceInitData> initData) override;
};
} // namespace Ship
