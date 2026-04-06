#pragma once

#include <stdint.h>

typedef int32_t EventID;
typedef int32_t ListenerID;

typedef enum {
    EVENT_PRIORITY_LOW,
    EVENT_PRIORITY_NORMAL,
    EVENT_PRIORITY_HIGH,
} EventPriority;

typedef struct {
    bool Cancelled;
} IEvent;

typedef void (*EventCallback)(IEvent*);

typedef struct EventMetadata {
    const char* Path;
    int Line;
    uint64_t Count;
} EventMetadata;

typedef struct EventListener {
    EventPriority Priority;
    EventCallback Function;
    EventMetadata Metadata;
} EventListener;

#ifndef __cplusplus
#ifdef INIT_EVENT_IDS
#define DECLARE_EVENT(eventName) EventID eventName##ID = -1;
#else
#define DECLARE_EVENT(eventName) extern EventID eventName##ID;
#endif
#else
#ifdef INIT_EVENT_IDS
#define DECLARE_EVENT(eventName) extern "C" EventID eventName##ID = -1;
#else
#define DECLARE_EVENT(eventName) extern "C" EventID eventName##ID;
#endif
#endif

#define STRINGIFY_DETAIL(x) #x
#define TOSTRING(x) STRINGIFY_DETAIL(x)

#define FILE_AND_LINE __FILE__ ":" TOSTRING(__LINE__)

#define DEFINE_EVENT(eventName, ...) \
    typedef struct {                 \
        IEvent Event;                \
        __VA_ARGS__                  \
    } eventName;                     \
                                     \
    DECLARE_EVENT(eventName)

#define CALL_EVENT(eventType, ...)                       \
    eventType eventType##_ = { { false }, __VA_ARGS__ }; \
    EventSystemCallEvent(eventType##ID, &eventType##_, __FILE__, __LINE__, FILE_AND_LINE);

#define CALL_CANCELLABLE_EVENT(eventType, ...)                                             \
    eventType eventType##_ = { { false }, __VA_ARGS__ };                                   \
    EventSystemCallEvent(eventType##ID, &eventType##_, __FILE__, __LINE__, FILE_AND_LINE); \
    if (!eventType##_.Event.Cancelled)

#define CHECK_IF_NOT_CANCELLED(eventType) if (!eventType##_.Event.Cancelled)

#define CALL_CANCELLABLE_RETURN_EVENT(eventType, ...)                                      \
    eventType eventType##_ = { { false }, __VA_ARGS__ };                                   \
    EventSystemCallEvent(eventType##ID, &eventType##_, __FILE__, __LINE__, FILE_AND_LINE); \
    if (eventType##_.Event.Cancelled) {                                                    \
        return;                                                                            \
    }

#define REGISTER_EVENT(eventType) eventType##ID = EventSystemRegisterEvent(#eventType);

#define REGISTER_LISTENER(eventType, priority, callback) \
    EventSystemRegisterListener(eventType##ID, callback, priority, __FILE__, __LINE__);

#define UNREGISTER_LISTENER(eventType, listenerID) EventSystemUnregisterListener(eventType##ID, listenerID);