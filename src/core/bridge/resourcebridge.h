#pragma once

#ifndef RESOURCEBRIDGE_H
#define RESOURCEBRIDGE_H

#include "stdint.h"

#ifdef __cplusplus
#include "resource/Archive.h"
#include "resource/type/Texture.h"
#include "resource/Resource.h"

std::shared_ptr<Ship::Resource> LoadResource(const char* name, bool now);
std::shared_ptr<Ship::Resource> LoadResource(uint64_t crc, bool now);
std::shared_ptr<Ship::Texture> GetResourceTexByName(const char* name);
std::shared_ptr<Ship::Texture> GetResourceTexByCrc(uint64_t crc);

extern "C" {
#endif

uint64_t GetResourceCrcByName(const char* name);
const char* GetResourceNameByCrc(uint64_t crc);
size_t GetResourceSizeByName(const char* name, bool now);
size_t GetResourceSizeByCrc(uint64_t crc, bool now);
void* GetResourceDataByName(const char* name, bool now);
void* GetResourceDataByCrc(uint64_t crc, bool now);
uint16_t GetResourceTexWidthByName(const char* name, bool now);
uint16_t GetResourceTexWidthByCrc(uint64_t crc, bool now);
uint16_t GetResourceTexHeightByName(const char* name, bool now);
uint16_t GetResourceTexHeightByCrc(uint64_t crc, bool now);
size_t GetResourceTexSizeByName(const char* name, bool now);
size_t GetResourceTexSizeByCrc(uint64_t crc, bool now);
void GetGameVersions(uint32_t* versions, size_t versionsSize, size_t* versionsCount);
uint32_t HasGameVersion(uint32_t hash);
void LoadResourceDirectory(const char* name);
void DirtyResourceDirectory(const char* name);
void DirtyResourceByName(const char* name);
void DirtyResourceByCrc(uint64_t crc);
size_t UnloadResourceByName(const char* name);
size_t UnloadResourceByCrc(uint64_t crc);
void ClearResourceCache(void);
void RegisterResourcePatchByName(const char* name, size_t index, uintptr_t origData, bool now);
void RegisterResourcePatchByCrc(uint64_t crc, size_t index, uintptr_t origData, bool now);
void WriteTextureDataInt16ByName(const char* name, size_t index, int16_t valueToWrite, bool now);
void WriteTextureDataInt16ByCrc(uint64_t crc, size_t index, int16_t valueToWrite, bool now);
uint8_t* GetTextureModifier(const char* texPath);
bool HasTextureModifier(const char* texPath);
void RegisterTextureModifier(char* texPath, uint8_t* modifier);
void RemoveTextureModifier(const char* texPath);

#ifdef __cplusplus
};
#endif

#endif
