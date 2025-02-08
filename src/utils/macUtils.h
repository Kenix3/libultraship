#ifndef macUtils_h
#define macUtils_h
#include <SDL.h>
#ifdef __cplusplus
extern "C" {
#endif
    void toggleNativeFullscreen(SDL_Window *window);
    bool isNativeFullscreenActive(SDL_Window *window);
#ifdef __cplusplus
}
#endif
#endif
