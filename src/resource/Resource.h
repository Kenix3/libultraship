#pragma once

#include <stdint.h>
#include "ResourceType.h"
#include "libultraship/version.h"
#include "binarytools/BinaryWriter.h"

namespace Ship {
class ResourceMgr;

struct ResourceAddressPatch {
    uint64_t ResourceCrc;
    uint32_t InstructionIndex;
    uintptr_t OriginalData;
};

struct ResourceInitData {
    std::string Path;
    Endianness ByteOrder;
    ResourceType Type;
    uint32_t ResourceVersion;
    uint64_t Id;
};

class Resource {
  public:
    std::shared_ptr<ResourceMgr> ResourceManager;
    std::shared_ptr<ResourceInitData> InitData;
    bool IsDirty = false;
    std::vector<ResourceAddressPatch> Patches;
    virtual void* GetPointer() = 0;
    virtual size_t GetPointerSize() = 0;
    Resource(std::shared_ptr<ResourceMgr> resourceManager, std::shared_ptr<ResourceInitData> initData);
    virtual ~Resource();
    void RegisterResourceAddressPatch(uint64_t crc, uint32_t instructionIndex, intptr_t originalData);
};
} // namespace Ship
