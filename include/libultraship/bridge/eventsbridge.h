#pragma once

#include "stdint.h"
#include "ship/events/EventTypes.h"
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers a new event type under the given name and returns its unique ID.
 *
 * If an event with @p name has already been registered the existing ID is returned.
 *
 * @param name Human-readable event name (e.g. "OnLoadGame").
 * @return EventID that can be passed to EventSystemRegisterListener() and EventSystemCallEvent().
 */
API_EXPORT EventID EventSystemRegisterEvent(const char* name);

/**
 * @brief Registers a listener callback for the given event.
 *
 * @param id       Event to listen to (obtained from EventSystemRegisterEvent()).
 * @param callback Function invoked when the event fires.
 * @param priority Relative priority; higher-priority listeners are called first.
 * @param file     Source file of the caller (use the @c __FILE__ macro).
 * @param line     Source line of the caller (use the @c __LINE__ macro).
 * @return ListenerID that can be passed to EventSystemUnregisterListener() to remove this listener.
 */
API_EXPORT ListenerID EventSystemRegisterListener(EventID id, EventCallback callback, EventPriority priority,
                                                  const char* file, int line);

/**
 * @brief Removes a previously registered listener.
 *
 * @param ev Event the listener was registered for.
 * @param id ListenerID returned by EventSystemRegisterListener().
 */
API_EXPORT void EventSystemUnregisterListener(EventID ev, ListenerID id);

/**
 * @brief Fires an event synchronously, invoking all registered listeners in priority order.
 *
 * @param id    Event to fire.
 * @param event Pointer to event-specific data passed to each listener callback.
 * @param file  Source file of the caller (use the @c __FILE__ macro).
 * @param line  Source line of the caller (use the @c __LINE__ macro).
 * @param key   Optional de-duplication key; pass nullptr for unconditional dispatch.
 */
API_EXPORT void EventSystemCallEvent(EventID id, void* event, const char* file, int line, const char* key);

#ifdef __cplusplus
}
#endif
