#include "ship/actions/DrawDebugMenuAction.h"

namespace Ship {

DrawDebugMenuAction::DrawDebugMenuAction(std::shared_ptr<Tickable> tickable)
    : Action(static_cast<uint32_t>(ActionType::DrawDebugMenu), tickable) {
}

bool DrawDebugMenuAction::ActionRan(const double durationSinceLastTick) {
    return true;
}

} // namespace Ship
