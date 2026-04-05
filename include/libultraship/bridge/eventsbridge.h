#pragma once

#include "stdint.h"
#include "ship/events/EventTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

extern EventID EventSystemRegisterEvent(const char* name);
extern ListenerID EventSystemRegisterListener(EventID id, EventCallback callback, EventPriority priority,
                                              const char* file, int line);
extern void EventSystemUnregisterListener(EventID ev, ListenerID id);
extern void EventSystemCallEvent(EventID id, void* event, const char* file, int line, const char* key);

#ifdef __cplusplus
}
#endif
