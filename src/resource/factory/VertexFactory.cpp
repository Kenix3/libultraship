#include "resource/factory/VertexFactory.h"
#include "resource/type/Vertex.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryVertexV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto vertex = std::make_shared<Vertex>(file->InitData);
    auto reader = std::get<std::shared_ptr<BinaryReader>>(file->Reader);

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

    return vertex;
}

std::shared_ptr<IResource> ResourceFactoryXMLVertexV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto vertex = std::make_shared<Vertex>(file->InitData);

    auto child =
        std::get<std::shared_ptr<tinyxml2::XMLDocument>>(file->Reader)->FirstChildElement()->FirstChildElement();

    while (child != nullptr) {
        std::string childName = child->Name();

        if (childName == "Vtx") {
            Vtx data;
            data.v.ob[0] = child->IntAttribute("X");
            data.v.ob[1] = child->IntAttribute("Y");
            data.v.ob[2] = child->IntAttribute("Z");
            data.v.flag = 0;
            data.v.tc[0] = child->IntAttribute("S");
            data.v.tc[1] = child->IntAttribute("T");
            data.v.cn[0] = child->IntAttribute("R");
            data.v.cn[1] = child->IntAttribute("G");
            data.v.cn[2] = child->IntAttribute("B");
            data.v.cn[3] = child->IntAttribute("A");

            vertex->VertexList.push_back(data);
        }

        child = child->NextSiblingElement();
    }

    return vertex;
}
} // namespace LUS
