#pragma once

#include "resource/Resource.h"

namespace LUS {
class Blob : public Resource {
  public:
    using Resource::Resource;

    Blob();

    void* GetRawPointer();
    size_t GetPointerSize();

    std::vector<uint8_t> Data;
};
}; // namespace LUS
