#include "resource/factory/VertexFactory.h"
#include "resource/type/Vertex.h"
#include "resource/readerbox/BinaryReaderBox.h"
#include "resource/readerbox/XMLReaderBox.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryVertexV0::ReadResource(std::shared_ptr<File> file) {
    if (file->InitData->Format != RESOURCE_FORMAT_BINARY) {
        SPDLOG_ERROR("resource file format does not match factory format.");
        return nullptr;
    }

    if (file->Reader == nullptr) {
        SPDLOG_ERROR("Failed to load resource: File has Reader ({} - {})", file->InitData->Type,
                        file->InitData->Path);
        return nullptr;
    }

    auto vertex = std::make_shared<Vertex>(initData);

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
    if (file->InitData->Format != RESOURCE_FORMAT_XML) {
        SPDLOG_ERROR("resource file format does not match factory format.");
        return nullptr;
    }

    if (file->XmlDocument == nullptr) {
        SPDLOG_ERROR("Failed to load resource: File has no XML document ({} - {})", file->InitData->Type,
                        file->InitData->Path);
        return result;
    }

    auto vertex = std::make_shared<Vertex>(initData);

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
