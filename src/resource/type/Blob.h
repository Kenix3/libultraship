#pragma once

#include "resource/Resource.h"

namespace Ship {
class Blob : public Ship::Resource<void> {
  public:
    using Resource::Resource;

    Blob();

    void* GetPointer() override;
    size_t GetPointerSize() override;

    std::vector<uint8_t> Data;
};
}; // namespace Ship
