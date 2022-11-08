#include "core/resourcebridge.h"
#include "core/Window.h"
#include "misc/Hooks.h"
#include "resource/ResourceMgr.h"
#include <string>

#include "resource/types/Vertex.h"
#include "graphic/Fast3D/U64/PR/ultra64/gbi.h"
#include "resource/types/Array.h"
#include "resource/types/Texture.h"
#include "resource/types/Blob.h"
#include "resource/types/Matrix.h"
#include "resource/types/DisplayList.h"

extern "C" {
#define LOAD_TEX(texPath) \
    static_cast<Ship::Texture*>(Ship::Window::GetInstance()->GetResourceManager()->LoadResource(texPath).get());

const char* ResourceMgr_GetNameByCRC(uint64_t crc) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(crc);
    return hashStr != nullptr ? hashStr->c_str() : nullptr;
}

Vtx* ResourceMgr_LoadVtxByCRC(uint64_t crc) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(crc);

    if (hashStr != nullptr) {
        auto res = std::static_pointer_cast<Ship::Array>(
            Ship::Window::GetInstance()->GetResourceManager()->LoadResource(hashStr->c_str()));
        return (Vtx*)res->vertices.data();
    }

    return nullptr;
}

int32_t* ResourceMgr_LoadMtxByCRC(uint64_t crc) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(crc);

    if (hashStr != nullptr) {
        auto res = std::static_pointer_cast<Ship::Matrix>(
            Ship::Window::GetInstance()->GetResourceManager()->LoadResource(hashStr->c_str()));
        return (int32_t*)res->mtx.data();
    }

    return nullptr;
}

Gfx* ResourceMgr_LoadGfxByCRC(uint64_t crc) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(crc);

    if (hashStr != nullptr) {
        auto res = std::static_pointer_cast<Ship::DisplayList>(
            Ship::Window::GetInstance()->GetResourceManager()->LoadResource(hashStr->c_str()));
        return (Gfx*)&res->instructions[0];
    } else {
        return nullptr;
    }
}

char* ResourceMgr_LoadTexByCRC(uint64_t crc) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(crc);

    if (hashStr != nullptr) {
        const auto res = LOAD_TEX(hashStr->c_str());
        Ship::ExecuteHooks<Ship::LoadTexture>(hashStr->c_str(), &res->imageData);

        return reinterpret_cast<char*>(res->imageData);
    } else {
        return nullptr;
    }
}

void ResourceMgr_RegisterResourcePatch(uint64_t hash, uint32_t instrIndex, uintptr_t origData) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(hash);

    if (hashStr != nullptr) {
        auto res =
            (Ship::Texture*)Ship::Window::GetInstance()->GetResourceManager()->LoadResource(hashStr->c_str()).get();

        Ship::Patch patch;
        patch.Crc = hash;
        patch.Index = instrIndex;
        patch.OrigData = origData;

        res->Patches.push_back(patch);
    }
}

char* ResourceMgr_LoadTexByName(char* texPath) {
    const auto res = LOAD_TEX(texPath);
    Ship::ExecuteHooks<Ship::LoadTexture>(texPath, &res->imageData);
    return (char*)res->imageData;
}

uint16_t ResourceMgr_LoadTexWidthByName(char* texPath) {
    const auto res = LOAD_TEX(texPath);
    if (res != nullptr) {
        return res->width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t ResourceMgr_LoadTexHeightByName(char* texPath) {
    const auto res = LOAD_TEX(texPath);
    if (res != nullptr) {
        return res->height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint32_t ResourceMgr_LoadTexSizeByName(char* texPath) {
    const auto res = LOAD_TEX(texPath);
    if (res != nullptr) {
        return res->imageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

void ResourceMgr_WriteTexS16ByName(char* texPath, size_t index, int16_t value) {
    const auto res = LOAD_TEX(texPath);

    if (res != nullptr) {
        if ((index * 2) < res->imageDataSize) {
            ((s16*)res->imageData)[index] = value;
        } else {
            // Dangit Morita
            int bp = 0;
        }
    }
}

char* ResourceMgr_LoadBlobByName(char* blobPath) {
    auto res = (Ship::Blob*)Ship::Window::GetInstance()->GetResourceManager()->LoadResource(blobPath).get();
    return (char*)res->data.data();
}
}