#pragma once

#include "stdint.h"
#include "ship/events/EventTypes.h"
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif

API_EXPORT EventID EventSystemRegisterEvent(const char* name);
API_EXPORT ListenerID EventSystemRegisterListener(EventID id, EventCallback callback, EventPriority priority,
                                              const char* file, int line);
API_EXPORT void EventSystemUnregisterListener(EventID ev, ListenerID id);
API_EXPORT void EventSystemCallEvent(EventID id, void* event, const char* file, int line, const char* key);

#ifdef __cplusplus
}
#endif
