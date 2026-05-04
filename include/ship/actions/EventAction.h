#pragma once

#include "ship/Action.h"

namespace Ship {

/**
 * @brief Generic Action that delegates to a TickableComponent for any event type.
 *
 * EventAction replaces the former TickAction, DrawAction, and DrawDebugMenuAction
 * classes. It delegates execution to the owning TickableComponent's ActionRan()
 * virtual, passing the EventID so the component can dispatch accordingly.
 *
 * EventIDs are registered dynamically with the Events component via REGISTER_EVENT.
 */
class EventAction : public Action {
  public:
    /**
     * @brief Constructs an EventAction for the given EventID.
     * @param eventId The EventID this action handles.
     * @param tickable  The Tickable that owns this Action.
     */
    EventAction(EventID eventId, std::shared_ptr<Tickable> tickable);
    virtual ~EventAction() = default;

  protected:
    /**
     * @brief Delegates to TickableComponent::ActionRan() with the EventID.
     * @param durationSinceLastTick Elapsed time in seconds since the last tick.
     * @return True if the action executed successfully.
     */
    bool ActionRan(const double durationSinceLastTick) override;
};

} // namespace Ship
