#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <string>

#include "ship/core/TickableComponent.h"
#include "ship/events/EventTypes.h"
#include "ship/events/ListenerAction.h"

namespace Ship {

/**
 * @brief Tracks registration state for a single EventID.
 *
 * Each record stores the event's debug name, listener allocation counters,
 * caller metadata for diagnostics, and the currently registered
 * `ListenerAction`s keyed by `ListenerID`.
 */
struct EventRegistration {
    /** @brief Optional human-readable name for this event. */
    const char* Name;
    /** @brief Monotonically increasing listener identifier counter for this event. */
    ListenerID NextListenerID = 0;
    /** @brief Registration-order sequence used to preserve stable listener ordering. */
    uint64_t NextListenerSequence = 0;
    /** @brief Diagnostic map of event call sites keyed by file:line string. */
    std::unordered_map<std::string, EventMetadata> Callers;
    /** @brief Active listeners for this event keyed by ListenerID. */
    std::unordered_map<ListenerID, std::shared_ptr<ListenerAction>> Listeners;
};

/**
 * @brief Tickable event bus that dispatches listeners through `Tick(EventID)`.
 *
 * `Events` now owns all registered listeners as `ListenerAction`s in its
 * `ActionList`. Calling `CallEvent()` pushes a transient `DispatchContext`,
 * ticks the matching `EventID`, and each `ListenerAction` pulls the active
 * payload from that context during execution.
 *
 * EventID contract:
 * - `-1` = uninitialized / invalid
 * - `< -1` = internally registered events returned by `RegisterEvent()`
 * - `>= 0` = user-defined events created on demand when listeners/calls occur
 *
 * Listeners are invoked synchronously in priority order whenever `CallEvent()`
 * is called.
 * Any listener may set `IEvent::Cancelled` to true; `CallEvent()` does not
 * enforce cancellation itself � the caller is responsible for checking it via
 * the `CALL_CANCELLABLE_EVENT` macro.
 *
 * **Required Context children:** None � `Events` has no dependencies on other
 * components.
 *
 * Obtain the instance from `Context::GetChildren().GetFirst<Events>()`.
 *
 * Typical usage (C++ side):
 * @code
 * // Registration (once at startup)
 * REGISTER_EVENT(MyEvent);
 *
 * // Listening
 * REGISTER_LISTENER(MyEvent, EVENT_PRIORITY_NORMAL, [](IEvent* e) {
 *     auto* event = static_cast<MyEvent*>(e);
 *     // handle...
 * });
 *
 * // Dispatching
 * CALL_EVENT(MyEvent, payload);
 * @endcode
 */
class Events : public TickableComponent {
  public:
    /**
     * @brief Active payload and call-site data for a single in-flight dispatch.
     *
     * A new context is pushed for each `CallEvent()` invocation so nested event
     * dispatch remains well-defined.
     */
    struct DispatchContext {
        /** @brief EventID currently being dispatched. */
        EventID ID = -1;
        /** @brief Event payload supplied by the caller. */
        IEvent* Event = nullptr;
        /** @brief Source file that initiated the dispatch, when provided. */
        const char* File = nullptr;
        /** @brief Source line that initiated the dispatch, when provided. */
        int Line = 0;
        /** @brief Optional deduplication key used for caller diagnostics. */
        const char* Key = nullptr;
    };

    /**
     * @brief Constructs and self-initializes the global event bus component.
     *
     * The component starts ticking immediately so listener actions can be run as
     * soon as the `Events` instance is reachable through the active `Context`.
     */
    Events() : TickableComponent("Events", nullptr) {
        MarkInitialized();
        Start();
    }

    /**
     * @brief Allocates a new internally managed EventID.
     * @param name Optional human-readable event name for diagnostics.
     * @return A negative EventID less than `-1`.
     */
    EventID RegisterEvent(const char* name = nullptr);

    /**
     * @brief Registers a callback listener for an event.
     *
     * For internally managed events (`id < -1`), the event must already have
     * been registered with `RegisterEvent()`. User-defined event IDs (`id >= 0`)
     * are created lazily when first referenced.
     *
     * @param id       EventID to subscribe to.
     * @param callback Listener callback invoked with the active `IEvent*`.
     * @param priority Dispatch priority; lower values run first.
     * @param file     Optional source file of the registration site.
     * @param line     Optional source line of the registration site.
     * @return ListenerID scoped to the target event.
     */
    ListenerID RegisterListener(EventID id, EventCallback callback, EventPriority priority = EVENT_PRIORITY_NORMAL,
                                const char* file = nullptr, int line = 0);

    /**
     * @brief Removes a previously registered listener from an event.
     *
     * Removing a listener also removes its backing `ListenerAction` from the
     * `Events` action list.
     *
     * @param id         EventID the listener belongs to.
     * @param listenerId ListenerID returned by `RegisterListener()`.
     */
    void UnregisterListener(EventID id, ListenerID listenerId);

    /**
     * @brief Removes an event and all listeners currently registered to it.
     * @param id EventID to remove.
     */
    void RemoveEvent(EventID id);

    /**
     * @brief Dispatches an event by pushing dispatch state and ticking its EventID.
     *
     * `ListenerAction`s matching @p id are executed through `Tick(id)` and read
     * the active payload from `GetActiveDispatchContext()`.
     *
     * @param id    EventID to dispatch.
     * @param event Event payload.
     * @param file  Optional source file of the call site.
     * @param line  Optional source line of the call site.
     * @param key   Optional caller-diagnostic key.
     */
    void CallEvent(EventID id, IEvent* event, const char* file = nullptr, int line = 0, const char* key = nullptr);

    /**
     * @brief Returns the registration record for an EventID, if present.
     * @param id EventID to look up.
     * @return Pointer to the registration, or `nullptr` if not found.
     */
    EventRegistration* GetEventRegistration(EventID id) {
        if (mEventRegistry.contains(id)) {
            return &mEventRegistry.at(id);
        }
        return nullptr;
    }

    /**
     * @brief Returns the complete event registry map.
     *
     * Intended for inspection and tooling such as the event debugger.
     */
    std::unordered_map<EventID, EventRegistration>& GetEventRegistrations() {
        return this->mEventRegistry;
    }

    /**
     * @brief Returns the current top-of-stack dispatch context.
     * @return Active dispatch context, or `nullptr` when no event is being dispatched.
     */
    const DispatchContext* GetActiveDispatchContext() const;

  private:
    /**
     * @brief Gets or lazily creates a registration record for an EventID.
     * @param id EventID to resolve.
     * @return Registration record, or `nullptr` when @p id is invalid.
     */
    EventRegistration* GetOrCreateEventRegistration(EventID id);

    /** @brief Full registry of known EventIDs. */
    std::unordered_map<EventID, EventRegistration> mEventRegistry;
    /** @brief Next internally allocated EventID; decrements from `-2`. */
    EventID mInternalEventID = -2;
    /** @brief Stack of active dispatch contexts used to support nested event calls. */
    std::vector<DispatchContext> mDispatchStack;
};

} // namespace Ship
