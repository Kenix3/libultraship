#pragma once

#include <stddef.h>
#include "ship/Api.h"

/**
 * @brief Callback type invoked by the crash handler when a crash is detected.
 *
 * The callback receives a mutable character buffer (@p char*) and the current
 * write offset within that buffer (@p size_t*). Implementations should append
 * any game-specific crash context and update the offset accordingly.
 */
typedef void (*CrashHandlerCallback)(char*, size_t*);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers an application callback to be invoked on crash.
 *
 * The callback is called after the CrashHandler has written the platform crash
 * report, giving the application an opportunity to append additional information
 * (e.g. current game state) before the process exits.
 *
 * @param callback Function to invoke on crash. Pass nullptr to clear the callback.
 */
API_EXPORT void CrashHandlerRegisterCallback(CrashHandlerCallback callback);

#ifdef __cplusplus
}
#endif
