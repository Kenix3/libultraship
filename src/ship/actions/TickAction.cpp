#include "ship/actions/TickAction.h"
#include "ship/TickableComponent.h"

namespace Ship {

TickAction::TickAction(std::shared_ptr<Tickable> tickable) : Action(static_cast<uint32_t>(ActionType::Tick), tickable) {
}

bool TickAction::ActionRan(const double durationSinceLastTick) {
    auto tickable = GetTickable();
    if (auto* tc = dynamic_cast<TickableComponent*>(tickable.get())) {
        return tc->ActionRan(static_cast<uint32_t>(ActionType::Tick), durationSinceLastTick);
    }
    return true;
}

} // namespace Ship
