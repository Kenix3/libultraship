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
 * @brief Checks whether the current process is running under Rosetta 2 (Apple's x86 translation layer).
 * @return true if current process is an x86 process running on an ARM CPU, false otherwise.
 */
bool isRunningUnderRosetta();

#ifdef __cplusplus
}
#endif
