#pragma once

#include "resource/Resource.h"

namespace LUS {
class Font : public Resource<void> {
  public:
    using Resource::Resource;

    Font();

    void* GetPointer() override;
    size_t GetPointerSize() override;

    std::vector<char> Data;
};
}; // namespace LUS
