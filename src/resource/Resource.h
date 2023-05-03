#pragma once

#include <stdint.h>
#include "ResourceType.h"
#include "libultraship/version.h"
#include "binarytools/BinaryWriter.h"

namespace Ship {
class ResourceManager;

struct ResourceInitData {
    std::string Path;
    Endianness ByteOrder;
    ResourceType Type;
    uint32_t ResourceVersion;
    uint64_t Id;
    bool IsCustom;
};

class Resource {
  public:
    inline static const std::string gAltAssetPrefix = "alt/";

    std::shared_ptr<Ship::ResourceManager> ResourceManager;
    std::shared_ptr<ResourceInitData> InitData;
    bool IsDirty = false;
    virtual void* GetPointer() = 0;
    virtual size_t GetPointerSize() = 0;
    Resource(std::shared_ptr<Ship::ResourceManager> resourceManager, std::shared_ptr<ResourceInitData> initData);
    virtual ~Resource();
};
} // namespace Ship
