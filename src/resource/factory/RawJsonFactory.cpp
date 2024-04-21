#include "resource/factory/RawJsonFactory.h"
#include "resource/type/RawJson.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryRawJsonV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto rawJson = std::make_shared<RawJson>(file->InitData);
    auto reader = std::get<std::shared_ptr<BinaryReader>>(file->Reader);

    rawJson->DataSize = file->Buffer->size();
    rawJson->Data = nlohmann::json::parse(reader->ReadCString(), nullptr, true, true);

    return rawJson;
}
} // namespace LUS
