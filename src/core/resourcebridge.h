#pragma once

#ifndef RESOURCEBRIDGE_H
#define RESOURCEBRIDGE_H

#include "stdint.h"
#include "graphic/Fast3D/U64/PR/ultra64/gbi.h"

#ifdef __cplusplus
extern "C" {
#endif

const char* ResourceMgr_GetNameByCRC(uint64_t crc);
Vtx* ResourceMgr_LoadVtxByCRC(uint64_t crc);
int32_t* ResourceMgr_LoadMtxByCRC(uint64_t crc);
Gfx* ResourceMgr_LoadGfxByCRC(uint64_t crc);
char* ResourceMgr_LoadTexByCRC(uint64_t crc);
void ResourceMgr_RegisterResourcePatch(uint64_t hash, uint32_t instrIndex, uintptr_t origData);
char* ResourceMgr_LoadTexByName(char* texPath);
uint16_t ResourceMgr_LoadTexWidthByName(char* texPath);
uint16_t ResourceMgr_LoadTexHeightByName(char* texPath);
uint32_t ResourceMgr_LoadTexSizeByName(char* texPath);
void ResourceMgr_WriteTexS16ByName(char* texPath, size_t index, int16_t value);
char* ResourceMgr_LoadBlobByName(char* blobPath);

#ifdef __cplusplus
};
#endif

#endif