#include "ship/actions/EventAction.h"
#include "ship/TickableComponent.h"

namespace Ship {

EventAction::EventAction(const std::string& eventName, std::shared_ptr<Tickable> tickable)
    : Action(eventName, tickable) {
}

bool EventAction::ActionRan(const double durationSinceLastTick) {
    auto tickable = GetTickable();
    if (auto* tc = dynamic_cast<TickableComponent*>(tickable.get())) {
        return tc->ActionRan(GetEventName(), durationSinceLastTick);
    }
    return true;
}

} // namespace Ship
