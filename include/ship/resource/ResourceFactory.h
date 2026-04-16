#pragma once

#include <memory>
#include "File.h"
#include <ship/resource/Resource.h>

namespace Ship {
/**
 * @brief Abstract factory interface for deserializing a specific resource type and format.
 *
 * Implement this class to add support for a new resource type. Register the implementation
 * with ResourceLoader::RegisterResourceFactory() so that it is invoked automatically when
 * a matching file is encountered.
 *
 * Example skeleton:
 * @code
 * class TextureFactory : public Ship::ResourceFactory {
 *   public:
 *     std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
 *                                                   std::shared_ptr<Ship::ResourceInitData> initData) override;
 *   protected:
 *     bool FileHasValidFormatAndReader(std::shared_ptr<Ship::File> file,
 *                                     std::shared_ptr<Ship::ResourceInitData> initData) override;
 * };
 * @endcode
 */
class ResourceFactory {
  public:
    /**
     * @brief Deserializes a File into an IResource.
     *
     * Called by ResourceLoader after it has selected this factory as the best match for
     * the file's (format, type, version) triple.
     *
     * @param file     Raw file data with a populated Reader (BinaryReader or XMLDocument).
     * @param initData Metadata parsed from the file header.
     * @return Fully constructed IResource, or nullptr if deserialization failed.
     */
    virtual std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file,
                                                    std::shared_ptr<ResourceInitData> initData) = 0;

  protected:
    /**
     * @brief Validates that the file's format tag and reader type are compatible with this factory.
     *
     * ResourceLoader calls this before invoking ReadResource(). Return false to signal that
     * the file is malformed or in an unexpected format, which will abort loading.
     *
     * @param file     Raw file data to inspect.
     * @param initData Metadata from the file header.
     * @return true if the file is valid and can be processed by this factory.
     */
    virtual bool FileHasValidFormatAndReader(std::shared_ptr<File> file,
                                             std::shared_ptr<Ship::ResourceInitData> initData) = 0;
};
} // namespace Ship
