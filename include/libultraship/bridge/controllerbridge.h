#pragma once

#include <stdint.h>
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Prevents controller gamepad input from being processed by the game.
 *
 * Multiple callers can independently block input; each must supply a unique @p inputBlockId
 * and must call ControllerUnblockGameInput() with the same ID to release its block.
 *
 * @param inputBlockId Caller-chosen identifier that distinguishes this blocker.
 */
API_EXPORT void ControllerBlockGameInput(uint16_t inputBlockId);

/**
 * @brief Releases a previously established game input block.
 *
 * Input is only fully unblocked once all outstanding blockers have called this function.
 *
 * @param inputBlockId The same ID passed to ControllerBlockGameInput().
 */
API_EXPORT void ControllerUnblockGameInput(uint16_t inputBlockId);

#ifdef __cplusplus
};
#endif
