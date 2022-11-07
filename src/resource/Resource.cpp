#include "Resource.h"
#include "resource/types/DisplayList.h"
#include "resource/ResourceMgr.h"
#include <spdlog/spdlog.h>
#include <tinyxml2.h>
#include "graphic/Fast3D/U64/PR/ultra64/gbi.h"

namespace Ship {
void ResourceFile::ParseFileBinary(BinaryReader* reader, Resource* res) {
    Id = reader->ReadUInt64();
    res->Id = Id;
    reader->ReadUInt32(); // Resource minor version number
    reader->ReadUInt64(); // ROM CRC
    reader->ReadUInt32(); // ROM Enum

    // Reserved for future file format versions...
    reader->Seek(64, SeekOffsetType::Start);
}
void ResourceFile::ParseFileXML(tinyxml2::XMLElement* reader, Resource* res) {
    Id = reader->Int64Attribute("id", -1);
}

void ResourceFile::WriteFileBinary(BinaryWriter* writer, Resource* res) {
}

void ResourceFile::WriteFileXML(tinyxml2::XMLElement* writer, Resource* res) {
}

Resource::~Resource() {
    free(CachedGameAsset);
    CachedGameAsset = nullptr;

    for (size_t i = 0; i < Patches.size(); i++) {
        const std::string* hashStr = ResourceManager->HashToString(Patches[i].Crc);
        if (hashStr == nullptr) {
            continue;
        }

        auto resShared = ResourceManager->GetCachedFile(hashStr->c_str());
        if (resShared != nullptr) {
            auto res = (Ship::DisplayList*)resShared.get();

            Gfx* gfx = &((Gfx*)res->instructions.data())[Patches[i].Index];
            gfx->words.w1 = Patches[i].OrigData;
        }
    }

    Patches.clear();

    if (File != nullptr) {
        SPDLOG_TRACE("Deconstructor called on file %s\n", file->path.c_str());
    }
}
} // namespace Ship