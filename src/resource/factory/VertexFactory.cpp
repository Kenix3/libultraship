#include "resource/factory/VertexFactory.h"
#include "resource/type/Vertex.h"
#include "spdlog/spdlog.h"

namespace Ship {
std::shared_ptr<Resource> VertexFactory::ReadResource(std::shared_ptr<BinaryReader> reader) {
    auto resource = std::make_shared<Vertex>();
    std::shared_ptr<ResourceVersionFactory> factory = nullptr;

    uint32_t version = reader->ReadUInt32();
    switch (version) {
        case 0:
            factory = std::make_shared<VertexFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load Vertex with version {}", version);
        return nullptr;
    }

    factory->ParseFileBinary(reader, resource);

    return resource;
}

void VertexFactoryV0::ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) {
    std::shared_ptr<Vertex> vertex = std::static_pointer_cast<Vertex>(resource);
    ResourceVersionFactory::ParseFileBinary(reader, vertex);

    uint32_t count = reader->ReadUInt32();
    vertex->VertexList.reserve(count);

    for (uint32_t i = 0; i < count; i++) {
        Vtx data;
        data.v.ob[0] = reader->ReadInt16();
        data.v.ob[1] = reader->ReadInt16();
        data.v.ob[2] = reader->ReadInt16();
        data.v.flag = reader->ReadUInt16();
        data.v.tc[0] = reader->ReadInt16();
        data.v.tc[1] = reader->ReadInt16();
        data.v.cn[0] = reader->ReadUByte();
        data.v.cn[1] = reader->ReadUByte();
        data.v.cn[2] = reader->ReadUByte();
        data.v.cn[3] = reader->ReadUByte();
        vertex->VertexList.push_back(data);
    }
}
} // namespace Ship
