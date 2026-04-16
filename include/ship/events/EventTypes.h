#pragma once

#include <stdint.h>
#include "ship/Api.h"

/** @brief Numeric identifier for a registered event; -1 is the uninitialized sentinel. */
typedef int32_t EventID;
/** @brief Numeric identifier for a registered listener; used to unregister later. */
typedef int64_t ListenerID;

/**
 * @brief Priority levels that control listener dispatch order within an event.
 *
 * Listeners with a higher priority value are invoked before those with a lower value.
 */
typedef enum {
    EVENT_PRIORITY_LOW,    ///< Invoked last; suitable for fallback or logging handlers.
    EVENT_PRIORITY_NORMAL, ///< Default priority for most listeners.
    EVENT_PRIORITY_HIGH,   ///< Invoked first; suitable for interception or override handlers.
} EventPriority;

/**
 * @brief Base event payload struct.
 *
 * All event-specific payload structs must embed this as their first member (see DEFINE_EVENT).
 * The Cancelled flag can be set by any listener to signal that subsequent handling
 * should be skipped; EventSystem itself does not enforce this — the caller uses the
 * CALL_CANCELLABLE_EVENT macro family to check the flag.
 */
typedef struct {
    /** @brief Set to true by a listener to indicate the event has been handled and should not propagate further. */
    bool Cancelled;
} IEvent;

/** @brief Callback signature for event listeners. The argument is the event payload cast to IEvent*. */
typedef void (*EventCallback)(IEvent*);

/**
 * @brief Diagnostic metadata recorded for each unique call site that fires an event.
 */
typedef struct EventMetadata {
    const char* Path; ///< Combined "__FILE__:__LINE__" string of the call site.
    int Line;         ///< Source line number of the call site.
    uint64_t Count;   ///< Number of times this call site has fired the event.
} EventMetadata;

/**
 * @brief Runtime record for a single event listener registration.
 */
typedef struct EventListener {
    ListenerID ID;          ///< Unique identifier assigned at registration time.
    EventPriority Priority; ///< Dispatch priority; higher values fire first.
    EventCallback Function; ///< The callback to invoke when the event fires.
    EventMetadata Metadata; ///< Diagnostic metadata about the registration site.
} EventListener;

/**
 * @defgroup EventMacros Event helper macros
 * @{
 *
 * These macros provide a convenient, type-safe way to declare, register, call, and
 * listen to events without having to reference EventIDs directly.
 *
 * **Declaring an event type:**
 * @code
 * DEFINE_EVENT(OnWindowResize, int32_t Width; int32_t Height;)
 * @endcode
 * This expands to a struct named `OnWindowResize` that embeds `IEvent` as its first
 * member, and an external `EventID` named `OnWindowResizeID`.
 *
 * **Registering and calling:**
 * @code
 * REGISTER_EVENT(OnWindowResize);
 * CALL_EVENT(OnWindowResize, 1920, 1080);
 * @endcode
 */

#ifdef INIT_EVENT_IDS
#ifdef __cplusplus
/** @brief Defines and initialises (to -1) the EventID variable for an event type. */
#define DECLARE_EVENT(eventName) extern "C" EventID eventName##ID = -1;
#else
#define DECLARE_EVENT(eventName) EventID eventName##ID = -1;
#endif
#else
/** @brief Declares (extern) the EventID variable for an event type defined elsewhere. */
#define DECLARE_EVENT(eventName) API_EXPORT EventID eventName##ID;
#endif

#define STRINGIFY_DETAIL(x) #x
#define TOSTRING(x) STRINGIFY_DETAIL(x)

/** @brief Produces a "file:line" string literal at the call site. */
#define FILE_AND_LINE __FILE__ ":" TOSTRING(__LINE__)

/**
 * @brief Declares a new event type and its associated EventID variable.
 *
 * @param eventName The name of the event struct (and its ID, `eventName##ID`).
 * @param ...       Additional struct fields appended after the embedded IEvent base.
 *
 * Example:
 * @code
 * DEFINE_EVENT(OnPlayerDeath, int32_t PlayerIndex;)
 * @endcode
 */
#define DEFINE_EVENT(eventName, ...) \
    typedef struct {                 \
        IEvent Event;                \
        __VA_ARGS__                  \
    } eventName;                     \
                                     \
    DECLARE_EVENT(eventName)

/**
 * @brief Fires an event without checking for cancellation.
 *
 * Constructs the event payload from @p ... and calls EventSystem::CallEvent().
 */
#define CALL_EVENT(eventType, ...)                       \
    eventType eventType##_ = { { false }, __VA_ARGS__ }; \
    EventSystemCallEvent(eventType##ID, &eventType##_, __FILE__, __LINE__, FILE_AND_LINE);

/**
 * @brief Fires an event and enters the following block only if the event was NOT cancelled.
 *
 * Usage:
 * @code
 * CALL_CANCELLABLE_EVENT(MyEvent, arg1, arg2) {
 *     // runs only if no listener cancelled the event
 * }
 * @endcode
 */
#define CALL_CANCELLABLE_EVENT(eventType, ...)                                             \
    eventType eventType##_ = { { false }, __VA_ARGS__ };                                   \
    EventSystemCallEvent(eventType##ID, &eventType##_, __FILE__, __LINE__, FILE_AND_LINE); \
    if (!eventType##_.Event.Cancelled)

/**
 * @brief Guard condition that is true when the previously fired cancellable event was NOT cancelled.
 *
 * Must be used after a CALL_CANCELLABLE_EVENT block if a second check is needed.
 */
#define CHECK_IF_NOT_CANCELLED(eventType) if (!eventType##_.Event.Cancelled)

/**
 * @brief Fires an event and returns from the enclosing function if the event was cancelled.
 */
#define CALL_CANCELLABLE_RETURN_EVENT(eventType, ...)                                      \
    eventType eventType##_ = { { false }, __VA_ARGS__ };                                   \
    EventSystemCallEvent(eventType##ID, &eventType##_, __FILE__, __LINE__, FILE_AND_LINE); \
    if (eventType##_.Event.Cancelled) {                                                    \
        return;                                                                            \
    }

/**
 * @brief Registers an event type with the global EventSystem, populating `eventName##ID`.
 *
 * Must be called once at startup before any CALL_EVENT or REGISTER_LISTENER uses of the event.
 */
#define REGISTER_EVENT(eventType) eventType##ID = EventSystemRegisterEvent(#eventType);

/**
 * @brief Registers a listener callback for an event type with the given priority.
 * @param eventType The event type name (its ID is `eventType##ID`).
 * @param priority  An EventPriority value.
 * @param callback  Function pointer or lambda matching the EventCallback signature.
 */
#define REGISTER_LISTENER(eventType, priority, callback) \
    EventSystemRegisterListener(eventType##ID, callback, priority, __FILE__, __LINE__);

/**
 * @brief Unregisters a listener using the ListenerID returned by REGISTER_LISTENER.
 * @param eventType  The event type name.
 * @param listenerID ListenerID to remove.
 */
#define UNREGISTER_LISTENER(eventType, listenerID) EventSystemUnregisterListener(eventType##ID, listenerID);

/** @} */ // end of EventMacros group