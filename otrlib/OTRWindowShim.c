#include "PR/ultra64/gbi.h"
#include "Lib/Fast3D/gfx_pc.h"
#include "Lib/Fast3D/gfx_sdl.h"
#include "Lib/Fast3D/gfx_opengl.h"
#include "Lib/Fast3D/gfx_window_manager_api.h"

void StupidShimThingy(struct GfxWindowManagerAPI** WmApi, struct GfxRenderingAPI** RenderingApi)
{
	*WmApi = &gfx_sdl;
	*RenderingApi = &gfx_opengl_api;
}