#include "ship/actions/EventAction.h"
#include "ship/TickableComponent.h"

namespace Ship {

EventAction::EventAction(EventID eventId, std::shared_ptr<Tickable> tickable)
    : Action(eventId, tickable) {
}

bool EventAction::ActionRan(const double durationSinceLastTick) {
    auto tickable = GetTickable();
    if (auto* tc = dynamic_cast<TickableComponent*>(tickable.get())) {
        return tc->ActionRan(GetEventId(), durationSinceLastTick);
    }
    return true;
}

} // namespace Ship
