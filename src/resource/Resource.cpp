#include "Resource.h"
#include "resource/types/DisplayList.h"
#include "resource/ResourceMgr.h"
#include <spdlog/spdlog.h>
#include <tinyxml2.h>
#include <libultraship/libultraship.h>

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

void Resource::RegisterResourceAddressPatch(uint64_t crc, uint32_t instructionIndex, intptr_t originalData) {
    ResourceAddressPatch patch;
    patch.ResourceCrc = crc;
    patch.InstructionIndex = instructionIndex;
    patch.OriginalData = originalData;

    Patches.push_back(patch);
}

Resource::~Resource() {
    free(CachedGameAsset);
    CachedGameAsset = nullptr;

    for (size_t i = 0; i < Patches.size(); i++) {
        const std::string* hashStr = ResourceManager->HashToString(Patches[i].ResourceCrc);
        if (hashStr == nullptr) {
            continue;
        }

        auto resShared = ResourceManager->GetCachedFile(hashStr->c_str());
        if (resShared != nullptr) {
            auto res = (Ship::DisplayList*)resShared.get();

            Gfx* gfx = &((Gfx*)res->instructions.data())[Patches[i].InstructionIndex];
            gfx->words.w1 = Patches[i].OriginalData;
        }
    }

    Patches.clear();

    if (File != nullptr) {
        SPDLOG_TRACE("Deconstructor called on file %s\n", File->Path.c_str());
    }
}
} // namespace Ship