#include "resource/factory/LightsFactory.h"
#include "resource/type/Light.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> LightsFactory::ReadResource(std::shared_ptr<ResourceInitData> initData,
                                                            std::shared_ptr<BinaryReader> reader) {
    auto resource = std::make_shared<ExLight>(initData);
    std::shared_ptr<ResourceVersionFactory> factory = nullptr;

    switch (resource->GetInitData()->ResourceVersion) {
        case 0:
            factory = std::make_shared<LightsFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load Lights with version {}", resource->GetInitData()->ResourceVersion);
        return nullptr;
    }

    factory->ParseFileBinary(reader, resource);

    return resource;
}

void LightsFactoryV0::ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<IResource> resource) {
    std::shared_ptr<ExLight> light = std::static_pointer_cast<ExLight>(resource);

    ResourceVersionFactory::ParseFileBinary(reader, light);

    light->data = new Lights_X;
    reader->Read(reinterpret_cast<char*>(light->data->ambient.l.col), 3);
    reader->Read(reinterpret_cast<char*>(light->data->ambient.l.colc), 3);

    const uint32_t size = reader->ReadUInt32();

    for(size_t i = 0; i < size; i++) {
        Light l;

        reader->Read(reinterpret_cast<char*>(l.l.col), 3);
        reader->Read(reinterpret_cast<char*>(l.l.colc), 3);
        reader->Read(reinterpret_cast<char*>(l.l.dir), 3);

        light->lights.push_back(l);
    }

    light->data->lights = light->lights.data();
}
} // namespace LUS
