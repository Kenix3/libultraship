#ifndef macUtils_h
#define macUtils_h
#include <SDL.h>
#ifdef __cplusplus
extern "C" {
#endif
void toggleNativeMacOSFullscreen(SDL_Window* window);
bool isNativeMacOSFullscreenActive(SDL_Window* window);
#ifdef __cplusplus
}
#endif
#endif
