#include "ship/actions/DrawAction.h"
#include "ship/TickableComponent.h"

namespace Ship {

DrawAction::DrawAction(std::shared_ptr<Tickable> tickable) : Action(static_cast<uint32_t>(ActionType::Draw), tickable) {
}

bool DrawAction::ActionRan(const double durationSinceLastTick) {
    auto tickable = GetTickable();
    if (auto* tc = dynamic_cast<TickableComponent*>(tickable.get())) {
        return tc->ActionRan(static_cast<uint32_t>(ActionType::Draw), durationSinceLastTick);
    }
    return true;
}

} // namespace Ship
