#include "Resource.h"
#include "resource/type/DisplayList.h"
#include "resource/ResourceMgr.h"
#include <spdlog/spdlog.h>
#include <tinyxml2.h>
#include "libultraship/libultra/gbi.h"

namespace Ship {
void Resource::RegisterResourceAddressPatch(uint64_t crc, uint32_t instructionIndex, intptr_t originalData) {
    ResourceAddressPatch patch;
    patch.ResourceCrc = crc;
    patch.InstructionIndex = instructionIndex;
    patch.OriginalData = originalData;

    Patches.push_back(patch);
}

Resource::~Resource() {
    for (size_t i = 0; i < Patches.size(); i++) {
        const std::string* hashStr = ResourceManager->HashToString(Patches[i].ResourceCrc);
        if (hashStr == nullptr) {
            continue;
        }

        auto resShared = ResourceManager->GetCachedFile(hashStr->c_str());
        if (resShared != nullptr) {
            auto res = (Ship::DisplayList*)resShared.get();

            Gfx* gfx = &((Gfx*)res->Instructions.data())[Patches[i].InstructionIndex];
            gfx->words.w1 = Patches[i].OriginalData;
        }
    }

    Patches.clear();

    if (File != nullptr) {
        SPDLOG_TRACE("Deconstructor called on file %s\n", File->Path.c_str());
    }
}
} // namespace Ship
