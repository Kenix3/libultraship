#pragma once

#include <cstdint>

#include "ship/actions/EventAction.h"

namespace Ship {

class Events;

/**
 * @brief Event-bus listener action executed by `Events::Tick(EventID)`.
 *
 * `ListenerAction` is the payload-aware action type used by the global
 * `Events` component. It extends `EventAction` with listener identity,
 * dispatch priority, registration order, and registration-site metadata.
 *
 * During `ActionRan()`, the action reads the currently active payload from the
 * owning `Events` component's dispatch context and invokes its stored
 * `EventCallback`.
 */
class ListenerAction : public EventAction {
  public:
    /**
     * @brief Constructs a listener action for a single EventID.
     * @param eventId    EventID this listener subscribes to.
     * @param listenerId Event-scoped listener identifier.
     * @param priority   Dispatch priority; higher values run first.
     * @param sequence   Stable registration-order sequence within the event.
     * @param tickable   Owning `Events` tickable.
     * @param callback   Listener callback invoked with the active event payload.
     * @param metadata   Registration-site diagnostic metadata.
     */
    ListenerAction(EventID eventId, ListenerID listenerId, EventPriority priority, uint64_t sequence,
                   std::shared_ptr<Tickable> tickable, EventCallback callback,
                   EventMetadata metadata = { nullptr, 0, 0 });

    /** @brief Returns the event-scoped ListenerID for this action. */
    ListenerID GetListenerId() const;
    /** @brief Returns the dispatch priority used for ordering listeners. */
    EventPriority GetPriority() const;
    /** @brief Returns the stable registration-order sequence number. */
    uint64_t GetSequence() const;
    /** @brief Returns the registration-site metadata recorded for this listener. */
    const EventMetadata& GetMetadata() const;

  protected:
    /**
     * @brief Executes the listener callback against the current `Events` payload.
     * @return True when the listener was dispatched successfully.
     */
    bool ActionRan() override;

  private:
    /** @brief Event-scoped listener identifier. */
    ListenerID mListenerId;
    /** @brief Dispatch priority used by `ActionList` ordering. */
    EventPriority mPriority;
    /** @brief Stable registration-order sequence number. */
    uint64_t mSequence;
    /** @brief Callback invoked during event dispatch. */
    EventCallback mEventCallback;
    /** @brief Registration-site metadata for debugging and tooling. */
    EventMetadata mMetadata;
};

} // namespace Ship
