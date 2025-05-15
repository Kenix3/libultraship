#pragma once

#include "resource/Resource.h"

namespace Ship {
#define RESOURCE_TYPE_FONT 0x464F4E54 // FONT

class Font : public Resource<void> {
  public:
    using Resource::Resource;

    Font();
    virtual ~Font();

    void* GetPointer() override;
    size_t GetPointerSize() override;

    char* Data = nullptr;
    size_t DataSize;
};
}; // namespace Ship
