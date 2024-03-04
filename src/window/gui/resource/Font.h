#pragma once

#include "resource/Resource.h"

namespace LUS {
#define RESOURCE_TYPE_FONT 0x464F4E54 // FONT

class Font : public Resource<void> {
  public:
    using Resource::Resource;

    Font();
    ~Font();

    void* GetPointer() override;
    size_t GetPointerSize() override;

    char* Data = nullptr;
    size_t DataSize;
};
}; // namespace LUS
