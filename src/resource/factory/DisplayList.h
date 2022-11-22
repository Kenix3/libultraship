#pragma once

#include <vector>
#include "../Resource.h"
#include "../ResourceFactory.h"
#include "libultra/gbi.h"
#include <vector>

namespace Ship {
class DisplayListFactory : public ResourceFactory {
  public:
    std::shared_ptr<Resource> ReadResource(std::shared_ptr<BinaryReader> reader);
};

class DisplayListFactoryV0 : public ResourceVersionFactory {
  public:
    void ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) override;
};

class DisplayList : public Resource {
  public:
    void* GetPointer();
    size_t GetPointerSize();

    std::vector<Gfx> Instructions;
};
} // namespace Ship
