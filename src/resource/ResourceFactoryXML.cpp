#include "ResourceFactoryXML.h"
#include "spdlog/spdlog.h"

namespace LUS {
bool ResourceFactoryXML::FileHasValidFormatAndReader(std::shared_ptr<File> file) {
    if (file->InitData->Format != RESOURCE_FORMAT_XML) {
        SPDLOG_ERROR("resource file format does not match factory format.");
        return false;
    }

    if (file->XmlDocument == nullptr) {
        SPDLOG_ERROR("Failed to load resource: File has no XML document ({} - {})", file->InitData->Type,
                        file->InitData->Path);
        return false;
    }

    return true;
};
} // namespace LUS