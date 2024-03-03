#pragma once

#include "resource/Resource.h"

namespace LUS {
#define RESOURCE_TYPE_FONT 0x464F4E54

class Font : public Resource<void> {
  public:
    using Resource::Resource;

    Font();

    void* GetPointer() override;
    size_t GetPointerSize() override;

    std::vector<char> Data;
};
}; // namespace LUS
