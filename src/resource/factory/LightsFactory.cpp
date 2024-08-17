#include "resource/factory/LightsFactory.h"
#include "resource/type/Lights.h"
#include "spdlog/spdlog.h"
#include "libultraship/libultra/gbi.h"

namespace LUS {
std::shared_ptr<Ship::IResource> ResourceFactoryBinaryLightsV0::ReadResource(std::shared_ptr<Ship::File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto light = std::make_shared<Light>(file->InitData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    unsigned char col[3];

    col[0] = reader->ReadInt8();
    col[1] = reader->ReadInt8();
    col[2] = reader->ReadInt8();

    light->Lit.a.l.col[0] = col[0];
    light->Lit.a.l.col[1] = col[1];
    light->Lit.a.l.col[2] = col[2];

    light->Lit.a.l.colc[0] = col[0];
    light->Lit.a.l.colc[1] = col[1];
    light->Lit.a.l.colc[2] = col[2];

    auto size = reader->ReadUInt32();

    for (size_t i = 0; i < size; i++) {
        unsigned char diffuse[3];
        diffuse[0] = reader->ReadUByte();
        diffuse[1] = reader->ReadUByte();
        diffuse[2] = reader->ReadUByte();
        light->Lit.l[i].l.col[0] = diffuse[0];
        light->Lit.l[i].l.col[1] = diffuse[1];
        light->Lit.l[i].l.col[2] = diffuse[2];
        light->Lit.l[i].l.colc[0] = diffuse[0];
        light->Lit.l[i].l.colc[1] = diffuse[1];
        light->Lit.l[i].l.colc[2] = diffuse[2];
        light->Lit.l[i].l.dir[0] = reader->ReadInt8();
        light->Lit.l[i].l.dir[1] = reader->ReadInt8();
        light->Lit.l[i].l.dir[2] = reader->ReadInt8();
    }

    return light;
}
} // namespace LUS
