#pragma once

#include "../Resource.h"
#include "../ResourceFactory.h"
#include "libultra/types.h"

namespace Ship {
class MatrixFactory : public ResourceFactory {
  public:
    std::shared_ptr<Resource> ReadResource(std::shared_ptr<BinaryReader> reader);
};

class MatrixFactoryV0 : public ResourceVersionFactory {
  public:
    void ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) override;
};

class Matrix : public Resource {
  public:
    void* GetPointer();
    size_t GetPointerSize();

    Mtx Matrx;
};
} // namespace Ship
