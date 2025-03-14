#include "ResourceFactoryXML.h"
#include "spdlog/spdlog.h"

namespace Ship {
bool ResourceFactoryXML::FileHasValidFormatAndReader(std::shared_ptr<File> file,
                                                     std::shared_ptr<Ship::ResourceInitData> initData) {
    if (initData->Format != RESOURCE_FORMAT_XML) {
        SPDLOG_ERROR("resource file format does not match factory format.");
        return false;
    }

    if (!std::holds_alternative<std::shared_ptr<tinyxml2::XMLDocument>>(file->Reader)) {
        SPDLOG_ERROR("Failed to load resource: File has no XML document ({} - {})", initData->Type, initData->Path);
        return false;
    }

    return true;
};
} // namespace Ship
