// macUtils.mm
#ifdef __APPLE__
#import "macUtils.h"
#import <SDL_syswm.h>
#import <Cocoa/Cocoa.h>

//Just a simple function to toggle the native macOS fullscreen.
void toggleNativeMacOSFullscreen(SDL_Window *window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(window, &wmInfo)) {
        NSWindow *nswindow = wmInfo.info.cocoa.window;
        [nswindow toggleFullScreen:nil];
    }
}

//Just a simple function to check if we are in native macOS fullscreen mode. Needed to avoid the game from crashing
//when going from native to SDL fullscreening modes or getting other forms of breakage.
bool isNativeMacOSFullscreenActive(SDL_Window *window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(window, &wmInfo)) {
        NSWindow *nswindow = wmInfo.info.cocoa.window;
        return (([nswindow styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen);
    }
    return false;
}
#endif