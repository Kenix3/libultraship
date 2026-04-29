#pragma once

#include <ship/resource/ResourceFactory.h>

namespace Ship {
/**
 * @brief ResourceFactory specialization for XML-format resource files.
 *
 * Validates that the file carries an XMLDocument and that its format tag equals
 * RESOURCE_FORMAT_XML. Subclass this when implementing a factory that
 * deserializes an XML-based resource.
 */
class ResourceFactoryXML : public ResourceFactory {
  protected:
    /**
     * @brief Validates that the file uses the XML format and contains an XMLDocument.
     * @param file     Raw file data to inspect.
     * @param initData Metadata from the file header.
     * @return true if the format is XML and the reader is an XMLDocument.
     */
    bool FileHasValidFormatAndReader(std::shared_ptr<File> file,
                                     std::shared_ptr<Ship::ResourceInitData> initData) override;
};
} // namespace Ship
