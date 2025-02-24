#pragma once

#include "resource/Resource.h"
namespace Ship {

class Shader : public Resource<void> {
  public:
    using Resource::Resource;

    Shader();

    void* GetPointer() override;
    size_t GetPointerSize() override;

    std::string Data;
};
}; // namespace Ship
