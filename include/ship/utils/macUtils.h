#pragma once

#include <SDL.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Toggles the native macOS fullscreen mode for an SDL window.
 * @param window Pointer to the SDL_Window to toggle.
 */
void toggleNativeMacOSFullscreen(SDL_Window* window);

/**
 * @brief Checks whether a native macOS fullscreen transition is active for an SDL window.
 * @param window Pointer to the SDL_Window to query.
 * @return true if native macOS fullscreen is currently active, false otherwise.
 */
bool isNativeMacOSFullscreenActive(SDL_Window* window);

/**
 * @brief Checks whether the window is fully occluded (not being composited/displayed).
 * @param window Pointer to the SDL_Window to query.
 * @return true if the window is occluded, false if it is visible.
 */
bool isWindowOccluded(SDL_Window* window);

/**
 * @brief Opts the process out of App Nap so it keeps running at full speed (and keeps
 *        audio playing without stutter) while unfocused or hidden. Call once at startup.
 */
void disableMacOSAppNap(void);

#ifdef __cplusplus
}
#endif
