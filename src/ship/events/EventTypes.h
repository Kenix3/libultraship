#pragma once

typedef int32_t EventID;
typedef uint32_t ListenerID;

typedef enum {
    EVENT_PRIORITY_LOW,
    EVENT_PRIORITY_NORMAL,
    EVENT_PRIORITY_HIGH,
} EventPriority;

typedef struct {
    bool cancelled;
} IEvent;

typedef void (*EventCallback)(IEvent*);

typedef struct EventMetadata {
    const char* path;
    int line;
    uint64_t count;
} EventMetadata;

typedef struct EventListener {
    EventPriority priority;
    EventCallback function;
    EventMetadata metadata;
} EventListener;

#ifndef __cplusplus
#ifdef INIT_EVENT_IDS
#define DECLARE_EVENT(eventName) uint32_t eventName##ID = -1;
#else
#define DECLARE_EVENT(eventName) extern uint32_t eventName##ID;
#endif
#else
#ifdef INIT_EVENT_IDS
#define DECLARE_EVENT(eventName) extern "C" uint32_t eventName##ID = -1;
#else
#define DECLARE_EVENT(eventName) extern "C" uint32_t eventName##ID;
#endif
#endif

#define STRINGIFY_DETAIL(x) #x
#define TOSTRING(x) STRINGIFY_DETAIL(x)

#define FILE_AND_LINE __FILE__ ":" TOSTRING(__LINE__)

#define DEFINE_EVENT(eventName, ...) \
    typedef struct {                 \
        IEvent event;                \
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
    if (!eventType##_.event.cancelled)

#define CHECK_IF_NOT_CANCELLED(eventType) if (!eventType##_.event.cancelled)

#define CALL_CANCELLABLE_RETURN_EVENT(eventType, ...)                                      \
    eventType eventType##_ = { { false }, __VA_ARGS__ };                                   \
    EventSystemCallEvent(eventType##ID, &eventType##_, __FILE__, __LINE__, FILE_AND_LINE); \
    if (eventType##_.event.cancelled) {                                                    \
        return;                                                                            \
    }

#define REGISTER_EVENT(eventType) eventType##ID = EventSystemRegisterEvent(#eventType);

#define REGISTER_LISTENER(eventType, priority, callback) \
    EventSystemRegisterListener(eventType##ID, callback, priority, __FILE__, __LINE__);

#define UNREGISTER_LISTENER(eventType, listenerID) EventSystemUnregisterListener(eventType##ID, listenerID);