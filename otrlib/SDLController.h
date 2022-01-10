#pragma once
#include "OTRController.h"
#include <SDL2/SDL.h>

#define INVALID_SDL_CONTROLLER_GUID (std::string("00000000000000000000000000000000"))

namespace OtrLib {
	class SDLController : public OTRController {
		public:
			SDLController(int32_t dwControllerNumber);
			~SDLController();

			void Read(OSContPad* pad);

		protected:
			std::string GetControllerType();
			void SetButtonMapping(std::string szButtonName, int32_t dwScancode);
			std::string GetConfSection();
			std::string GetBindingConfSection();
			void CreateDefaultBinding(std::string ContGuid);

		private:
			std::string guid;
			SDL_GameController* Cont;
			std::map<int32_t, int16_t> ThresholdMapping;

			void LoadAxisThresholds();
			void NormalizeStickAxis(int16_t wAxisValueX, int16_t wAxisValueY, int16_t wAxisThreshold);
			bool Open();
			bool Close();
	};
}
