#include "ship/actions/DrawAction.h"

namespace Ship {

DrawAction::DrawAction(std::shared_ptr<Tickable> tickable)
    : Action(static_cast<uint32_t>(ActionType::Draw), tickable) {}

bool DrawAction::ActionRan(const double durationSinceLastTick) {
    return true;
}

} // namespace Ship
