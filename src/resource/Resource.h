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

    Resource(std::shared_ptr<ResourceInitData> initData);
    virtual ~Resource();

    virtual void* GetRawPointer() = 0;
    virtual size_t GetPointerSize() = 0;

    bool IsDirty();
    void Dirty();
    std::shared_ptr<ResourceInitData> GetInitData();

  private:
    std::shared_ptr<ResourceInitData> mInitData;
    bool mIsDirty = false;
};


} // namespace LUS
