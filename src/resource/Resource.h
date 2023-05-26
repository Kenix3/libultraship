#pragma once

#include <stdint.h>
#include "ResourceType.h"
#include "utils/binarytools/BinaryWriter.h"

namespace LUS {
class ResourceManager;

struct ResourceInitData {
    std::string Path;
    Endianness ByteOrder;
    ResourceType Type;
    int32_t ResourceVersion;
    uint64_t Id;
    bool IsCustom;
};

class Resource {
  public:
    inline static const std::string gAltAssetPrefix = "alt/";

    std::shared_ptr<LUS::ResourceManager> ResourceManager;
    std::shared_ptr<ResourceInitData> InitData;
    bool IsDirty = false;
    virtual void* GetRawPointer() = 0;
    virtual size_t GetPointerSize() = 0;
    Resource(std::shared_ptr<ResourceInitData> initData);
    virtual ~Resource();
};
} // namespace LUS
