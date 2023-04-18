#pragma once

#include <stdint.h>
#include "ResourceType.h"
#include "libultraship/version.h"
#include "binarytools/BinaryWriter.h"

namespace Ship {
class ResourceMgr;

struct ResourceInitData {
    std::string Path;
    Endianness ByteOrder;
    ResourceType Type;
    uint32_t ResourceVersion;
    uint64_t Id;
    bool isCustom;
};

class Resource {
  public:
    std::shared_ptr<ResourceMgr> ResourceManager;
    std::shared_ptr<ResourceInitData> InitData;
    bool IsDirty = false;
    virtual void* GetPointer() = 0;
    virtual size_t GetPointerSize() = 0;
    Resource(std::shared_ptr<ResourceMgr> resourceManager, std::shared_ptr<ResourceInitData> initData);
    virtual ~Resource();
};
} // namespace Ship
