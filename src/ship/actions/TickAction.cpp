#include "ship/actions/TickAction.h"

namespace Ship {

TickAction::TickAction(std::shared_ptr<Tickable> tickable)
    : Action(static_cast<uint32_t>(ActionType::Tick), tickable) {}

bool TickAction::ActionRan(const double durationSinceLastTick) {
    return true;
}

} // namespace Ship
