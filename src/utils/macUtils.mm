// macUtils.mm
#import "macUtils.h"
#import <SDL_syswm.h>
#import <Cocoa/Cocoa.h>

void toggleNativeFullscreen(SDL_Window *window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(window, &wmInfo)) {
        NSWindow *nswindow = wmInfo.info.cocoa.window;
        [nswindow toggleFullScreen:nil];
    }
}

bool isNativeFullscreenActive(SDL_Window *window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(window, &wmInfo)) {
        NSWindow *nswindow = wmInfo.info.cocoa.window;
        return (([nswindow styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen);
    }
    return false;
}
