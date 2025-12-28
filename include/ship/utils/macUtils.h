#pragma once

#include <SDL.h>
#ifdef __cplusplus
extern "C" {
#endif
void toggleNativeMacOSFullscreen(SDL_Window* window);
bool isNativeMacOSFullscreenActive(SDL_Window* window);
#ifdef __cplusplus
}
#endif
