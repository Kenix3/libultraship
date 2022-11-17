#pragma once

#ifndef RESOURCEBRIDGE_H
#define RESOURCEBRIDGE_H

#include "stdint.h"

#ifdef __cplusplus
#include "resource/Resource.h"

std::shared_ptr<Ship::Resource> LoadResource(const char* name);
std::shared_ptr<Ship::Resource> LoadResource(uint64_t crc);

extern "C" {
#endif

uint64_t GetResourceCrcByName(const char* name);
const char* GetResourceNameByCrc(uint64_t crc);
size_t GetResourceSizeByName(const char* name);
size_t GetResourceSizeByCrc(uint64_t crc);
void* GetResourceDataByName(const char* name);
void* GetResourceDataByCrc(uint64_t crc);
uint16_t GetResourceTexWidthByName(const char* name);
uint16_t GetResourceTexWidthByCrc(uint64_t crc);
uint16_t GetResourceTexHeightByName(const char* name);
uint16_t GetResourceTexHeightByCrc(uint64_t crc);
size_t GetResourceTexSizeByName(const char* name);
size_t GetResourceTexSizeByCrc(uint64_t crc);
uint32_t GetGameVersion(void);
void LoadResourceDirectory(const char* name);
void DirtyResourceDirectory(const char* name);
void DirtyResourceByName(const char* name);
void DirtyResourceByCrc(uint64_t crc);
size_t UnloadResourceByName(const char* name);
size_t UnloadResourceByCrc(uint64_t crc);
void ClearResourceCache(void);
void RegisterResourcePatchByName(const char* name, size_t index, uintptr_t origData);
void RegisterResourcePatchByCrc(uint64_t crc, size_t index, uintptr_t origData);
void WriteTextureDataInt16ByName(const char* name, size_t index, int16_t valueToWrite);
void WriteTextureDataInt16ByCrc(uint64_t crc, size_t index, int16_t valueToWrite);

#ifdef __cplusplus
};
#endif

#endif