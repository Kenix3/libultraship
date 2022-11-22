#pragma once

#include <stdint.h>
#include "OtrFile.h"
#include "ResourceType.h"
#include "ResourceVersion.h"
#include "binarytools/BinaryWriter.h"
#include <spdlog/spdlog.h>

namespace Ship {
class ResourceMgr;

struct ResourceAddressPatch {
    uint64_t ResourceCrc;
    uint32_t InstructionIndex;
    uintptr_t OriginalData;
};

class Resource {
  public:
    std::shared_ptr<ResourceMgr> ResourceManager;
    Endianness ByteOrder;      // 0x00 - Endianness of the file
    ResourceType ResourceType; // 0x01 - 4 byte MAGIC
    ResourceVersion Ver;       // 0x05 - Based on Ship release numbers
    uint64_t Id;               // 0x09 - Unique Resource ID
    uint32_t ResourceVersion;  // 0x11 - Resource Minor Version Number    bool IsDirty = false;
    bool IsDirty = false;
    std::shared_ptr<OtrFile> File;
    std::vector<ResourceAddressPatch> Patches;
    virtual void* GetPointer() = 0;
    virtual size_t GetPointerSize() = 0;
    virtual ~Resource();
    void RegisterResourceAddressPatch(uint64_t crc, uint32_t instructionIndex, intptr_t originalData);
};

class ResourcePromise {
  public:
    std::shared_ptr<Resource> Res;
    std::shared_ptr<OtrFile> File;
    std::condition_variable ResourceLoadNotifier;
    std::mutex ResourceLoadMutex;
    bool HasResourceLoaded = false;
};
} // namespace Ship
