#include "resource/factory/VertexFactory.h"
#include "resource/type/Vertex.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryVertexV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader()) {
        return nullptr;
    }

    auto vertex = std::make_shared<Vertex>(file->InitData);

    uint32_t count = file->Reader->ReadUInt32();
    vertex->VertexList.reserve(count);

    for (uint32_t i = 0; i < count; i++) {
        Vtx data;
        data.v.ob[0] = file->Reader->ReadInt16();
        data.v.ob[1] = file->Reader->ReadInt16();
        data.v.ob[2] = file->Reader->ReadInt16();
        data.v.flag = file->Reader->ReadUInt16();
        data.v.tc[0] = file->Reader->ReadInt16();
        data.v.tc[1] = file->Reader->ReadInt16();
        data.v.cn[0] = file->Reader->ReadUByte();
        data.v.cn[1] = file->Reader->ReadUByte();
        data.v.cn[2] = file->Reader->ReadUByte();
        data.v.cn[3] = file->Reader->ReadUByte();
        vertex->VertexList.push_back(data);
    }

    return vertex;
}

std::shared_ptr<IResource> ResourceFactoryXMLVertexV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader()) {
        return nullptr;
    }

    auto vertex = std::make_shared<Vertex>(file->InitData);

    auto child = file->XmlDocument->FirstChildElement()->FirstChildElement();

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
