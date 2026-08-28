// macUtils.mm
#ifdef __APPLE__
#import "ship/utils/macUtils.h"
#import <Cocoa/Cocoa.h>

//Just a simple function to toggle the native macOS fullscreen.
void toggleNativeMacOSFullscreen(SDL_Window *window) {
    NSWindow *nswindow = (__bridge NSWindow *)SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
    if (nswindow != nil) {
        [nswindow toggleFullScreen:nil];
    }
}

//Just a simple function to check if we are in native macOS fullscreen mode. Needed to avoid the game from crashing
//when going from native to SDL fullscreening modes or getting other forms of breakage.
bool isNativeMacOSFullscreenActive(SDL_Window *window) {
    NSWindow *nswindow = (__bridge NSWindow *)SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
    if (nswindow != nil) {
        return (([nswindow styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen);
    }
    return false;
}

// Opt out of App Nap. When the app isn't frontmost (unfocused or hidden) macOS
// otherwise throttles its timers and CPU scheduling across *all* threads, which
// slows the game loop and starves the audio thread (stutter). Registering a
// latency-critical activity keeps the process running at full speed in the
// background. The token is held for the app's lifetime via a strong static (ARC).
void disableMacOSAppNap(void) {
    static id<NSObject> sAppNapActivity = nil;
    if (sAppNapActivity == nil) {
        sAppNapActivity = [[NSProcessInfo processInfo]
            beginActivityWithOptions:(NSActivityUserInitiatedAllowingIdleSystemSleep | NSActivityLatencyCritical)
                              reason:@"libultraship keeps simulating and outputs audio while unfocused"];
    }
}
#endif
