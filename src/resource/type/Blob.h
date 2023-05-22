#pragma once

#include "resource/Resource.h"

namespace LUS {
class Blob : public Resource {
  public:
    using Resource::Resource;

    Blob() : Resource(std::shared_ptr<ResourceInitData>()) {}

    void* GetPointer();
    size_t GetPointerSize();

    std::vector<uint8_t> Data;
};
}; // namespace LUS
