#pragma once

#include <cstdint>
#include "resource/Resource.h"

namespace LUS {

struct LightN64 {
    uint8_t col[3];
    int8_t pad1;
    uint8_t colc[3];
    int8_t pad2;
    uint8_t dir[3];
    int8_t pad3;
};

struct PointLightN64 {
    uint8_t col[3];
    uint8_t unk3;
    uint8_t colc[3];
    uint8_t unk7;
    int16_t pos[3];
    uint8_t unkE;
};

union LightData {
    LightN64 l;
    PointLightN64 p;
    long long int force_structure_alignment[2];
};

union Ambient {
    uint8_t col[3];
    int8_t pad1;
    uint8_t colc[3];
    int8_t pad2;
};

struct LightEntry {
    Ambient a;
    LightData l;
};

class Light : public Ship::Resource<LightEntry> {
  public:
    using Resource::Resource;

    Light() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {
    }

    LightEntry* GetPointer();
    size_t GetPointerSize();

    LightEntry mLight;
    std::vector<LightData> mLightData;
};
} // namespace LUS