// macUtils.mm
#ifdef __APPLE__
#import "ship/utils/macUtils.h"
#import <SDL_syswm.h>
#import <Cocoa/Cocoa.h>
#include <sys/sysctl.h>

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

//Whether the current process is running under Rosetta 2 translation.
bool isRunningUnderRosetta() {
    int translated = 0;
    size_t size = sizeof(translated);
    if (sysctlbyname("sysctl.proc_translated", &translated, &size, nullptr, 0) == 0)
        return translated;
    return false;
}
#endif
