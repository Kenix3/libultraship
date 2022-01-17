#include "PR/ultra64/gbi.h"
#include "Lib/Fast3D/gfx_pc.h"
#include "Lib/Fast3D/gfx_sdl.h"
#include "Lib/Fast3D/gfx_opengl.h"
#include "Lib/Fast3D/gfx_window_manager_api.h"

/*
 * Begin shims for gfx_pc.c. Eventually, a file from SOH repo should be moved in here.
 */
char* ResourceMgr_GetNameByCRC(uint64_t crc, char* alloc) {
    return NULL;
}

Vtx* ResourceMgr_LoadVtxByCRC(uint64_t crc, char* alloc, int allocSize){
	return NULL;
}

Gfx* ResourceMgr_LoadGfxByCRC(uint64_t crc) {
	return NULL;
}

char* ResourceMgr_LoadTexOriginalByCRC(uint64_t crc, char* alloc) {
	return NULL;
}

void ToggleConsole() {
	
}
/*
 * End empty shims
 */

void SetWindowManager(struct GfxWindowManagerAPI** WmApi, struct GfxRenderingAPI** RenderingApi) {
	*WmApi = &gfx_sdl;
	*RenderingApi = &gfx_opengl_api;
}