#include "Resource.h"
#include "resource/type/DisplayList.h"
#include "resource/ResourceMgr.h"
#include <spdlog/spdlog.h>
#include "libultraship/libultra/gbi.h"

namespace Ship {
Resource::Resource(std::shared_ptr<ResourceMgr> resourceManager, std::shared_ptr<ResourceInitData> initData)
    : ResourceManager(resourceManager), InitData(initData) {
}

Resource::~Resource() {
    for (size_t i = 0; i < Patches.size(); i++) {
        const std::string* hashStr = ResourceManager->HashToString(Patches[i].ResourceCrc);
        if (hashStr == nullptr) {
            continue;
        }

        auto patchedResource = ResourceManager->GetCachedResource(*hashStr);
        if (patchedResource != nullptr) {
            auto dl = static_pointer_cast<DisplayList>(patchedResource);
            if (dl != nullptr) {
                Gfx* gfx = &(dl->Instructions.data())[Patches[i].InstructionIndex];
                gfx->words.w1 = Patches[i].OriginalData;
            } else {
                SPDLOG_WARN("Failed to unpatch resource {} during resource {} unload.", patchedResource->InitData->Path,
                            InitData->Path);
            }
        } else {
            SPDLOG_WARN("Failed to get cached resource {} to unpatch during resource {} unload.", *hashStr,
                        InitData->Path);
        }
    }

    Patches.clear();

    SPDLOG_TRACE("Resource Unloaded: {}\n", InitData->Path);
}

void Resource::RegisterResourceAddressPatch(uint64_t crc, uint32_t instructionIndex, intptr_t originalData) {
    ResourceAddressPatch patch;
    patch.ResourceCrc = crc;
    patch.InstructionIndex = instructionIndex;
    patch.OriginalData = originalData;

    Patches.push_back(patch);
}
} // namespace Ship
