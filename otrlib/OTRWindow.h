#pragma once
#include <memory>
#include <thread>
#include "PR/ultra64/gbi.h"
#include "Lib/Fast3D/gfx_pc.h"
#include "Lib/Fast3D/gfx_sdl.h"
#include "Lib/Fast3D/gfx_opengl.h"
#include "UltraController.h"
#include "OTRController.h"
#include "OTRContext.h"

namespace OtrLib {
	class OTRWindow {
		public:
			static std::shared_ptr<OtrLib::OTRController> Controllers[MAXCONTROLLERS];

			OTRWindow(std::shared_ptr<OTRContext> Context);
			void MainLoop(void (*MainFunction)(void));
			void Init();
			void RunCommands(Gfx* Commands);
			void SetFrameDivisor(int divisor);

			bool IsFullscreen() { return bIsFullscreen; }
			int32_t GetResolutionX();
			int32_t GetResolutionY();

		protected:
		private:
			static bool KeyDown(int32_t dwScancode);
			static bool KeyUp(int32_t dwScancode);
			static void AllKeysUp(void);
			static void OnFullscreenChanged(bool bIsNowFullscreen);

			std::shared_ptr<OTRContext> Context;

			GfxWindowManagerAPI* WmApi;
			GfxRenderingAPI* RenderingApi;
			bool bIsFullscreen;
			uint32_t dwWidth;
			uint32_t dwHeight;
	};
}

