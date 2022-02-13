#pragma once
#include <memory>
#include "PR/ultra64/gbi.h"
#include "Lib/Fast3D/gfx_pc.h"
#include "UltraController.h"
#include "Controller.h"
#include "GlobalCtx2.h"

namespace Ship {
	class Window {
		public:
			static std::shared_ptr<Ship::Controller> Controllers[MAXCONTROLLERS];

			Window(std::shared_ptr<GlobalCtx2> Context);
			~Window();
			void MainLoop(void (*MainFunction)(void));
			void Init();
			void RunCommands(Gfx* Commands);
			void SetFrameDivisor(int divisor);
			uint16_t GetPixelDepth(float x, float y);
			void ToggleFullscreen();
			void SetFullscreen(bool bIsFullscreen);

			bool IsFullscreen() { return bIsFullscreen; }
			uint32_t GetCurrentWidth();
			uint32_t GetCurrentHeight();
			std::shared_ptr<GlobalCtx2> GetContext() { return Context.lock(); }
			static int32_t lastScancode;

		protected:
		private:
			static bool KeyDown(int32_t dwScancode);
			static bool KeyUp(int32_t dwScancode);
			static void AllKeysUp(void);
			static void OnFullscreenChanged(bool bIsNowFullscreen);

			std::weak_ptr<GlobalCtx2> Context;

			GfxWindowManagerAPI* WmApi;
			GfxRenderingAPI* RenderingApi;
			bool bIsFullscreen;
			uint32_t dwWidth;
			uint32_t dwHeight;
	};
}

