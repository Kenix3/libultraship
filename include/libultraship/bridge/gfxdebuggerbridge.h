#pragma once

#include <stddef.h>
#include <stdint.h>
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Requests that the graphics debugger begin capturing the next display list.
 *
 * Sets an internal flag that is checked at the start of the next frame. Once capture
 * begins, GfxDebuggerIsDebugging() returns true until the capture is complete.
 */
API_EXPORT void GfxDebuggerRequestDebugging();

/**
 * @brief Returns true while the graphics debugger is actively capturing a display list.
 */
API_EXPORT bool GfxDebuggerIsDebugging();

/**
 * @brief Returns true if a debug capture has been requested but has not yet started.
 */
API_EXPORT bool GfxDebuggerIsDebuggingRequested();

/**
 * @brief Passes a display list to the graphics debugger for inspection.
 *
 * Should be called once per frame while GfxDebuggerIsDebugging() is true.
 *
 * @param cmds Pointer to the head of the display list command buffer.
 */
API_EXPORT void GfxDebuggerDebugDisplayList(void* cmds);

#ifdef __cplusplus
};
#endif
