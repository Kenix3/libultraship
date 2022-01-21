#include "PR/ultra64/gbi.h"
#include "Lib/Fast3D/gfx_pc.h"
#include "Lib/Fast3D/gfx_sdl.h"
#include "Lib/Fast3D/gfx_dxgi.h"
#include "Lib/Fast3D/gfx_glx.h"
#include "Lib/Fast3D/gfx_opengl.h"
#include "Lib/Fast3D/gfx_direct3d11.h"
#include "Lib/Fast3D/gfx_direct3d12.h"
#include "Lib/Fast3D/gfx_window_manager_api.h"

/*
 * Begin shims for gfx_pc.c. Eventually, a file from SOH repo should be moved in here.
 */

//void ToggleConsole() 
//{
//	
//}
/*
 * End empty shims
 */

void SetWindowManager(struct GfxWindowManagerAPI** WmApi, struct GfxRenderingAPI** RenderingApi) {
#if defined(ENABLE_DX12)
    *RenderingApi = &gfx_direct3d12_api;
    *WmApi = &gfx_dxgi_api;
#elif defined(ENABLE_DX11)
    *RenderingApi = &gfx_direct3d11_api;
    *WmApi = &gfx_dxgi_api;
#elif defined(ENABLE_OPENGL)
    *RenderingApi = &gfx_opengl_api;
	#if defined(__linux__)
	    *WmApi = &gfx_glx;
	#else
	    *WmApi = &gfx_sdl;
	#endif
#endif
}