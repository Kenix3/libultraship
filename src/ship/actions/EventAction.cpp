#include "ship/actions/EventAction.h"
#include "ship/core/TickableComponent.h"

namespace Ship {

EventAction::EventAction(EventID eventId, std::shared_ptr<Tickable> tickable)
    : Action(tickable), mEventId(eventId) {
}

EventID EventAction::GetEventId() const {
    return mEventId;
}

bool EventAction::ActionRan(const double durationSinceLastTick) {
    auto tickable = GetTickable();
    if (auto* tc = dynamic_cast<TickableComponent*>(tickable.get())) {
        return tc->ActionRan(mEventId, durationSinceLastTick);
    }
    return true;
}

} // namespace Ship
