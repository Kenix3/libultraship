#pragma once

#ifdef ENABLE_PRESS_TO_JOIN

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void MultiplayerStart(uint8_t portCount);
void MultiplayerStopPressToJoin(void);
void MultiplayerStartPressToJoin(void);
void MultiplayerStop(void);
int8_t MultiplayerGetPortStatus(uint8_t port);

#ifdef __cplusplus
};
#endif

#endif // ENABLE_PRESS_TO_JOIN
