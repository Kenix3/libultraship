#include "resource/factory/JsonFactory.h"
#include "resource/type/Json.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<ShipDK::IResource> ResourceFactoryBinaryJsonV0::ReadResource(std::shared_ptr<ShipDK::File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto json = std::make_shared<Json>(file->InitData);
    auto reader = std::get<std::shared_ptr<ShipDK::BinaryReader>>(file->Reader);

    json->DataSize = file->Buffer->size();
    json->Data = nlohmann::json::parse(reader->ReadCString(), nullptr, true, true);

    return json;
}
} // namespace LUS
