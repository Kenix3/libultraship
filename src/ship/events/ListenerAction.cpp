#include "ship/events/ListenerAction.h"

#include "ship/events/Events.h"

namespace Ship {

ListenerAction::ListenerAction(EventID eventId, ListenerID listenerId, EventPriority priority, uint64_t sequence,
                               std::shared_ptr<Tickable> tickable, EventCallback callback, EventMetadata metadata)
    : EventAction(eventId, std::move(tickable)), mListenerId(listenerId), mPriority(priority), mSequence(sequence),
      mEventCallback(callback), mMetadata(metadata) {
}

ListenerID ListenerAction::GetListenerId() const {
    return mListenerId;
}

EventPriority ListenerAction::GetPriority() const {
    return mPriority;
}

uint64_t ListenerAction::GetSequence() const {
    return mSequence;
}

const EventMetadata& ListenerAction::GetMetadata() const {
    return mMetadata;
}

bool ListenerAction::ActionRan() {
    if (mEventCallback == nullptr) {
        return true;
    }

    auto tickable = GetTickable();
    auto* events = dynamic_cast<Events*>(tickable.get());
    if (events == nullptr) {
        return false;
    }

    const auto* dispatchContext = events->GetActiveDispatchContext();
    if (dispatchContext == nullptr || dispatchContext->Event == nullptr || dispatchContext->ID != GetEventId()) {
        return false;
    }

    mEventCallback(dispatchContext->Event);
    return true;
}

} // namespace Ship
