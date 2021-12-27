#pragma once
#include <memory>
#include <thread>
#include <SDL2/SDL.h>

namespace OtrLib {
	class OTRInput {
		public:
			static std::shared_ptr<OTRInput> Input;
			static bool StartInput();
			static bool StopInput();
			void operator()();
		protected:
		private:
			OTRInput();
			void HandleEvent(SDL_Event e);

			std::thread* Thread;
			volatile bool bIsRunning;
	};
}

