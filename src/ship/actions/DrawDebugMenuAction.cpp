#include "ship/actions/DrawDebugMenuAction.h"
#include "ship/TickableComponent.h"

namespace Ship {

DrawDebugMenuAction::DrawDebugMenuAction(std::shared_ptr<Tickable> tickable)
    : Action(static_cast<uint32_t>(ActionType::DrawDebugMenu), tickable) {
}

bool DrawDebugMenuAction::ActionRan(const double durationSinceLastTick) {
    auto tickable = GetTickable();
    if (auto* tc = dynamic_cast<TickableComponent*>(tickable.get())) {
        return tc->ActionRan(static_cast<uint32_t>(ActionType::DrawDebugMenu), durationSinceLastTick);
    }
    return true;
}

} // namespace Ship
