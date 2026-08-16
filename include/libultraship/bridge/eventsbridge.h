#pragma once

#include "stdint.h"
#include "ship/events/EventTypes.h"
#include "ship/Api.h"

#ifdef __cplusplus
#include <memory>
namespace Ship {
class Events;
}
/**
 * @brief Sets the global `Events` instance used by the C bridge.
 * @param events Shared `Events` component to forward bridge calls to.
 */
void EventSystemSetEvents(std::shared_ptr<Ship::Events> events);
/** @brief Returns the current bridged `Events` component, if any. */
std::shared_ptr<Ship::Events> EventSystemGetEvents();
extern "C" {
#endif

/**
 * @brief Registers a new internally managed event type under the given name.
 *
 * Returned IDs are negative values less than `-1`. User-defined event IDs are
 * represented by `0` and positive values and do not need to be allocated here.
 *
 * @param name Human-readable event name (e.g. "OnLoadGame").
 * @return Negative EventID that can be passed to listener registration and dispatch.
 */
API_EXPORT EventID EventSystemRegisterEvent(const char* name);

/**
 * @brief Registers a listener callback for the given event.
 *
 * Negative IDs must already be registered through `EventSystemRegisterEvent()`.
 * Zero and positive IDs are treated as user-defined IDs and are created lazily.
 *
 * @param id       Event to listen to.
 * @param callback Function invoked when the event fires.
 * @param priority Relative priority; higher-priority listeners are called first.
 * @param file     Source file of the caller (use the @c __FILE__ macro).
 * @param line     Source line of the caller (use the @c __LINE__ macro).
 * @return ListenerID that can be passed to `EventSystemUnregisterListener()`.
 */
API_EXPORT ListenerID EventSystemRegisterListener(EventID id, EventCallback callback, EventPriority priority,
                                                  const char* file, int line);

/**
 * @brief Removes an event and all listeners registered to it.
 * @param id Event to remove.
 */
API_EXPORT void EventSystemUnregisterEvent(EventID id);

/**
 * @brief Removes a previously registered listener.
 *
 * Removing the listener also removes its backing `ListenerAction` from the
 * `Events` component's action list.
 *
 * @param ev Event the listener was registered for.
 * @param id ListenerID returned by `EventSystemRegisterListener()`.
 */
API_EXPORT void EventSystemUnregisterListener(EventID ev, ListenerID id);

/**
 * @brief Fires an event synchronously.
 *
 * Internally this forwards to `Ship::Events::CallEvent()`, which pushes a
 * dispatch context and executes matching listeners by calling `Events->Tick(id)`.
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
