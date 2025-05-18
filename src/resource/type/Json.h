#pragma once

#include "resource/Resource.h"
#include <nlohmann/json.hpp>

namespace Ship {

class Json final : public Resource<void> {
  public:
    using Resource::Resource;

    Json();

    void* GetPointer() override;
    size_t GetPointerSize() override;

    nlohmann::json Data;
    size_t DataSize;
};
}; // namespace Ship
