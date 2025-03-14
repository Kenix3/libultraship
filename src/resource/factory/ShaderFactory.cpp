#include "resource/factory/ShaderFactory.h"
#include "spdlog/spdlog.h"

namespace Ship {
std::shared_ptr<IResource> ResourceFactoryBinaryShaderV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto shader = std::make_shared<Shader>(file->InitData);
    auto reader = std::get<std::shared_ptr<BinaryReader>>(file->Reader);

    shader->Data = reader->ReadCString();
    return shader;
}
} // namespace Ship
